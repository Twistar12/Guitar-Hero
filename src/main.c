#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

// tml library to parse midi files for note timings and lanes
// https://github.com/schellingb/TinySoundFont
#define TML_IMPLEMENTATION 
#include "tml.h"

#define MAX_NOTES 1000
#define LANE_COUNT 4
#define SONG_COUNT 3

typedef struct {
    int lane;       // Lanes 0,1,2,3,4
    float y;        // Vertical position
    bool active;    // Presence of the note on screen
    bool hit;       // Has the note been hit
    float hitTime;  // Precise time the note should hit
    bool spawned;   // Has the note been spawned
} Note;

typedef struct {
    const char *title;
    Color pColor;
    Color sColor;
    const char *songPath;   // Path to the song's audio file
    const char *imgPath;    // Path to the song's background image
    const char *filePath;   // Path to the .txt file containing note timings and lanes
    float offset;          // Add offset for calibration
} Song;

typedef enum {
    MENU, PLAYING, PAUSED, GAMEOVER
} GameState;

// GLOBAL VARIABLES
GameState currentState = MENU; // Default state menu
Song playlist[SONG_COUNT] = {
    {"Hacking to the Gate", RED, WHITE, "../resources/songs/steins-gate.mp3", "../resources/images/steins-gate.png", "../resources/beatmaps/steins-gate.mid", 0.0f},
    {"I Really Want to Stay at your House", BLUE, YELLOW, "../resources/songs/cyberpunk2077.mp3", "../resources/images/cyberpunk2077.png", "../resources/beatmaps/cyberpunk2077.mid", 0.40f},
    {"MEGALOVANIA", SKYBLUE, WHITE, "../resources/songs/megalovania.mp3", "../resources/images/megalovania.png", "../resources/beatmaps/megalovania.mid", 0.2f}
};

// SONG AND NOTE MANAGEMENT  
int selectedSong = 0; // Index of current song in playlist
Note notes[MAX_NOTES] = {0}; // To store notes and intialised to false
int score = 0;
int streak = 0; 
int maxStreak = 0; 
float receptorPulse[LANE_COUNT] = {0.0f}; // For visual feedback on receptors when hit

// Audio
Music currentMusic;         // raylib music struct 
bool musicLoaded = false;

// Background
Texture2D currentBg;
bool bgLoaded = false;

// Colors and Inputs corresponding to the fretboard
Color laneColours[LANE_COUNT] = { GREEN, RED, YELLOW, BLUE };
int keys[LANE_COUNT] = { KEY_D, KEY_F, KEY_J, KEY_K };

// Game Functions
// Song intialisation and control
void InitSong() {
    // Reset all variables
    score = 0;
    streak = 0;
    maxStreak = 0;
    for (int i = 0; i < MAX_NOTES; i++) {
        notes[i].active = false;
        notes[i].hit = false;
        notes[i].lane = 0;
        notes[i].hitTime = -1.0f;
        notes[i].spawned = false;
    }

    if (musicLoaded) {
        UnloadMusicStream(currentMusic);
        musicLoaded = false;
    }

    if (bgLoaded) {
        UnloadTexture(currentBg);
        bgLoaded = false;
    }
    
    // setup for playing mode
    // load MIDI file for parsing note times and lanes
    tml_message* midiData = tml_load_filename(playlist[selectedSong].filePath);
    if (midiData != NULL) {
        int noteIndex = 0;
        // Loop through every single MIDI event in chronological order
        // msg->next is pointer to next midi event (LINKED LIST), end is NULL
        for (tml_message* msg = midiData; msg != NULL; msg = msg->next) {
            
            // Check for NOTE ON events with existing velocity/volume
            if (msg->type == TML_NOTE_ON && msg->velocity > 0) {
                // Expert difficulty mapping 
                if (msg->key >= 96 && msg->key <= 99) {
                    notes[noteIndex].hitTime = msg->time / 1000.0f + playlist[selectedSong].offset; // Convert MIDI tick time to seconds
                    notes[noteIndex].lane = msg->key - 96;          // Map pitch to lanes (96 to 99 = 0 to 3)         
                    noteIndex++;
                    if (noteIndex >= MAX_NOTES) break; // prevent array overflow
                }
            }
        }
        // Free the memory once extracted what is needed from MIDI file
        tml_free(midiData); 
        printf("SUCCESS: Loaded MIDI beatmap with %d notes.\n", noteIndex);
            
        } else {
            printf("ERROR: Could not load the MIDI file %s!\n", playlist[selectedSong].filePath);
        }

    if (playlist[selectedSong].songPath[0] != '\0') {
        currentMusic = LoadMusicStream(playlist[selectedSong].songPath);
        currentMusic.looping = false;
        PlayMusicStream(currentMusic);
        musicLoaded = true;
    }
    if (playlist[selectedSong].imgPath[0] != '\0') {
        currentBg = LoadTexture(playlist[selectedSong].imgPath);
        bgLoaded = true;
    }
}

int main(void) {
    // Initialise the screen size occupied by the game
    int screenWidth = 1280;
    int screenHeight = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT); // Anti-aliasing for smoother visuals
    InitWindow(screenWidth, screenHeight, "Guitar Hero");
    ToggleFullscreen();
    InitAudioDevice(); // Required if porting the audio/song elements
    SetTargetFPS(60);

    // Load Textures and Audio for songs here
    Texture2D menuLogo = LoadTexture("../resources/images/guitar_hero_V1.png"); // menu logo image
    Texture2D menuBg = LoadTexture("../resources/images/menubg.png"); // menu background image

    // Game variables
    float noteSpeed = 350.0f;

    // Fretboard layout metrics
    float laneWidth = 80.0f;
    float startX = (screenWidth - (LANE_COUNT * laneWidth)) / 2.0f; // Center the lanes on the screen
    float startY = -50.0f;                 // spawn offscreen
    float hitY = screenHeight - 100.0f;    // Line at which notes can be hit (top to bottom)
    float hitTolerance = 50.0f;            // Time window to hit the note

    // Game Logic
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();   // Get delta time between previous and current frame in seconds
                                            // Compensate for frame drops and keep notes moving (consistency)
        if (currentState == PLAYING) {
            for (int i = 0; i < LANE_COUNT; i++) {
                // Decay receptor pulse after hit
                if (receptorPulse[i] > 0.0f) {
                    receptorPulse[i] -= deltaTime * 5.0f; // decay rate
                    if (receptorPulse[i] < 0.0f) receptorPulse[i] = 0.0f; // never go negative
                }
            }
        }

        int fps = GetFPS();
        // UPDATE LOGIC FOR NOTES
        if (currentState == MENU) {
            if (IsKeyPressed(KEY_DOWN)) selectedSong = (selectedSong + 1) % SONG_COUNT; // Cycle through songs
            if (IsKeyPressed(KEY_UP)) selectedSong = (selectedSong - 1 + SONG_COUNT) % SONG_COUNT; // Cycle backwards
            if (IsKeyPressed(KEY_ENTER)) {
                InitSong();
                currentState = PLAYING; // Select song to play
            }
        }
        else if (currentState == PLAYING) {
            if (IsKeyPressed(KEY_TAB)) currentState = PAUSED; // Pause the game
            if (musicLoaded) UpdateMusicStream(currentMusic); // Update music for continuous playback
            if (streak > maxStreak) maxStreak = streak; // Constantly update max streak

            // Check end of song, with slight buffer to catch for timing issues
            if  (musicLoaded && GetMusicTimePlayed(currentMusic) >= GetMusicTimeLength(currentMusic) - 0.1f) {
                currentState = GAMEOVER;
                StopMusicStream(currentMusic);
            }

            float currentSongTime = musicLoaded ? GetMusicTimePlayed(currentMusic) : 0.0f;
            float fallDistance = hitY - startY; // Distance from spawn point to hit line
            float fallTime = fallDistance / noteSpeed; // Time taken for note to fall to hitline (670 / 350 = 1.91s)

            // Spawn notes based on their hitTime and the current song time
            for (int i = 0; i < MAX_NOTES; i++) {
                if (notes[i].hitTime >= 0 && !notes[i].spawned) {
                    float spawnTime = notes[i].hitTime - fallTime; 

                    if (currentSongTime >= spawnTime) {
                        notes[i].active = true;
                        notes[i].spawned = true;
                        // Calculate note position based on time alive to prevent "jumping" during frame drops
                        float timeAlive = currentSongTime - spawnTime;
                        notes[i].y = startY + (timeAlive * noteSpeed); 
                    }
                }
            }

            // Move notes down the screen
            for (int i = 0; i < MAX_NOTES; i++) {
                if (notes[i].active) {
                    notes[i].y += noteSpeed * deltaTime; // Set position of notes based on time elapsed and not frames

                    // Check missed notes
                    if (notes[i].y > screenHeight + 50.0f) { // if note travels off screen
                        notes[i].active = false;        
                        streak = 0; // Reset streak on miss
                    }
                }
            }

            // Handle Guitar Input (D, F, J, K)
            for (int l = 0; l < LANE_COUNT; l++) {
                if (IsKeyPressed(keys[l])) {
                    bool hitSomething = false;
                    for (int i = 0; i < MAX_NOTES; i++) {
                        if (notes[i].active && notes[i].lane == l) { // if note is active and present on the current iterated lane
                            // Check if the note overlaps the strike line within time window
                            if (notes[i].y >= hitY - hitTolerance && notes[i].y <= hitY + hitTolerance) {
                                notes[i].active = false;
                                score += 10 + (streak / 10); // multiplier
                                streak++;
                                hitSomething = true;    
                                receptorPulse[l] = 1.0f; // Trigger pulse on receptor
                                break;
                            }
                        }
                    }
                    if (!hitSomething) {
                        streak = 0; // Prevent spamming presses and punish
                    }
                }
            }
        } else if (currentState == PAUSED) {
            if (IsKeyPressed(KEY_TAB)) currentState = PLAYING; // Resume the game
            if (IsKeyPressed(KEY_ENTER)) currentState = MENU; // Return to menu
        }
        else if (currentState == GAMEOVER) {
            if (IsKeyPressed(KEY_ENTER)) currentState = MENU; // Return to menu
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);

        // Select Song Menu
        if (currentState == MENU) {
            // Background image
            Rectangle bgSrc = {0, 0, (float)menuBg.width, (float)menuBg.height}; // Source rectangle
            Rectangle bgDest = {0, 0, (float)screenWidth, (float)screenHeight}; // Destination rectangle
            DrawTexturePro(menuBg, bgSrc, bgDest, (Vector2){0, 0}, 0, WHITE); // texture, src, dest, origin, rotation, tint
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f)); // Dark overlay 

            float logoScale = 0.5f; // Scale down the logo
            float scaledLogoWidth = menuLogo.width * logoScale;
            float scaledLogoHeight = menuLogo.height * logoScale;

            Vector2 logoPosition = { (screenWidth - scaledLogoWidth) / 2, 30 }; // Centered position vector for logo
            DrawTextureEx(menuLogo, logoPosition, 0, logoScale, WHITE); // texture, vector position, rotation, scale, tint

            int textStartY = logoPosition.y + scaledLogoHeight + 50; // Position text below the logo
            DrawText("SELECT A SONG:", (screenWidth - MeasureText("SELECT A SONG:", 40)) / 2, textStartY, 40 , WHITE); 
            
            // Song options
            for (int i = 0; i < SONG_COUNT; i++) {
                Color textColor = (i == selectedSong) ? playlist[i].pColor : WHITE; // Highlight selected song
                DrawText(playlist[i].title, (screenWidth - MeasureText(playlist[i].title, 30)) / 2, textStartY + 60 + (i * 50), 30, textColor);
            }

            // Navigation
            int controlsY = textStartY + 60 + (SONG_COUNT * 50) + 30; // Position controls text below song options
            DrawText("Controls: Use the keys [D, F, J, K] to hit the notes", (screenWidth - MeasureText("Controls: Use the keys [D, F, J, K] to hit the notes", 20)) / 2, controlsY, 20, GREEN);
            DrawText("Use UP/DOWN to navigate, ENTER to select", (screenWidth - MeasureText("Use UP/DOWN to navigate, ENTER to select", 20)) / 2, controlsY + 30, 20, LIGHTGRAY);
        }
        else if (currentState == PLAYING || currentState == GAMEOVER || currentState == PAUSED) {
            // Draw the background image for the current song
            if (bgLoaded) {
                // Set rectangle for bg and screen, to then resize bg to fit screen
                Rectangle bgRect = {0, 0, (float)currentBg.width, (float)currentBg.height}; // Source: select full image
                Rectangle screenRect = {0, 0, (float)screenWidth, (float)screenHeight}; // Dest: fullscreen
                DrawTexturePro(currentBg, bgRect, screenRect, (Vector2){0, 0}, 0, Fade(playlist[selectedSong].sColor, 0.5f)); // texture, src, dest, origin, rotation, tint
            }

            // Draw the lane separator lines for each lane
            for (int i = 0; i < LANE_COUNT; i++) {
                DrawRectangle(startX + (i * laneWidth), 0, laneWidth, screenHeight, Fade(BLACK, 0.7f)); // Faded lines for lanes

                // Thicker lane borders
                Vector2 lineStart = {startX + (i * laneWidth), 0};
                Vector2 lineEnd = {startX + (i * laneWidth), screenHeight};
                DrawLineEx(lineStart, lineEnd, 4.0f, Fade(playlist[selectedSong].pColor, 0.5f)); // start, end, thick, colour
            }

            // Lane Border at the end
            Vector2 finalLineStart = {startX + (LANE_COUNT * laneWidth), 0};
            Vector2 finalLineEnd = {startX + (LANE_COUNT * laneWidth), screenHeight};
            DrawLineEx(finalLineStart, finalLineEnd, 4.0f, Fade(playlist[selectedSong].pColor, 0.5f));

            // Draw strike line
            Vector2 strikeStart = {startX, hitY};
            Vector2 strikeEnd = {startX + (LANE_COUNT * laneWidth), hitY};
            DrawLineEx(strikeStart, strikeEnd, 5.0f, WHITE); // start, end, thick, colour

            for (int i = 0; i < LANE_COUNT; i++) {
                // Draw circle receptors
                Color C  = IsKeyDown(keys[i]) ? laneColours[i] : Fade(laneColours[i], 0.4f); // If current key is pressed, full colour else faded
                int centerX = (int)(startX + i * laneWidth + laneWidth / 2);

                float p = receptorPulse[i];
                float currentHitY = hitY - (p * 15.0f); // "Bounce" receptor up when hit based on pulse value
                float pScale = 1.0f + (p * 0.2f); // Scale up receptor when hit based on pulse value

                // Shockwave
                if (p > 0.0f) {
                    float ringScale = 1.0f + (1.0f - p); // Scale the shockwave based on pulse
                    DrawEllipseLines(centerX, (int)currentHitY, 35.0f * ringScale, 20.0f * ringScale, Fade(C, p * 0.8f)); // Expanding shockwave
                    DrawEllipse(centerX, (int)currentHitY, 35.0f * ringScale, 20.0f * ringScale, Fade(WHITE, p));         // Impact
                }
                DrawEllipse(centerX, (int)currentHitY, 35.0f * pScale, 20.0f * pScale, C);
                DrawEllipse(centerX, (int)currentHitY, 25.0f * pScale, 13.0f * pScale, BLACK);

                if (streak >= 15 && currentState != GAMEOVER) {
                    // Use sine wave to create a smooth pulsing math rhythm (GetTime() * speed)
                    // base + amplitude * sin(currentTime * speed) 
                    // sinwave from -1 to 1, amplitude range = -0.3 to 0.3, base added 0.3 so range is [0.3, 0.6]
                    float pulseAlpha = 0.3f + 0.3f * sin(GetTime() * 6.0f); 
                    
                    // Get the secondary color of the current song for the glow
                    Color glowColor = playlist[selectedSong].pColor; 
                    
                    // Draw a thick border around the entire screen for streaks of 15 (pulse effect)
                    Rectangle screenRect = {0, 0, screenWidth, screenHeight};
                    DrawRectangleLinesEx(screenRect, 10.0f, Fade(glowColor, pulseAlpha));
                    
                    // Milestone text for streak
                    DrawText(TextFormat("%d STREAK!", (streak / 15) * 15), screenWidth / 2 - 80, 50, 30, Fade(glowColor, pulseAlpha));
                }
            }
            
            if (currentState == GAMEOVER) {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f)); // Dark overlay for gameover
                DrawText("GAME OVER", (screenWidth - MeasureText("GAME OVER", 60)) / 2, screenHeight / 2 - 60, 60, RED);
                DrawText(TextFormat("Final Score: %06i", score), screenWidth / 2 - MeasureText(TextFormat("Final Score: %06i", score), 40) / 2, screenHeight / 2 + 10, 40, YELLOW);
                DrawText(TextFormat("Max Streak: %d", maxStreak), screenWidth / 2 - MeasureText(TextFormat("Max Streak: %d", maxStreak), 30) / 2, screenHeight / 2 + 60, 30, GREEN);
                DrawText("Press ENTER to return to menu", screenWidth / 2 - MeasureText("Press ENTER to return to menu", 20) / 2, screenHeight / 2 + 120, 20, LIGHTGRAY);
            }

            // Draw the active notes, hide when GAMEOVER
            if (currentState != GAMEOVER) {
                for (int i = 0; i < MAX_NOTES; i++) {
                    if (notes[i].active) { // draw "3d" pucks
                        int noteX = (int)(startX + notes[i].lane * laneWidth + laneWidth / 2);
                        int noteY = (int)notes[i].y;
                        Color noteColor = laneColours[notes[i].lane];

                        DrawEllipse(noteX, noteY + 4, 30.0f, 16.0f, WHITE); // base
                        DrawEllipse(noteX, noteY, 30.0f, 16.0f, noteColor); // top
                        DrawEllipse(noteX, noteY, 16.0f, 8.0f, BLACK); // inner ring
                        DrawEllipse(noteX, noteY, 8.0f, 4.0f, WHITE); // center highlight

                    }
                }
            }

            // UI
            DrawText(TextFormat("Score: %06i", score), 20, 20, 20, playlist[selectedSong].pColor); // Zero-padded score display
            DrawText(TextFormat("Streak: %d", streak), 20, 50, 20, playlist[selectedSong].sColor);
            DrawText(TextFormat("FPS: %d", fps), 20, 80, 20, WHITE);
            if (currentState == PAUSED) { // Overlay for paused state
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
                DrawText("PAUSED", screenWidth / 2 - MeasureText("PAUSED", 60) / 2, screenHeight / 2 - 30, 60, WHITE);
                DrawText("Press TAB to resume or ENTER to return to menu", screenWidth / 2 - MeasureText("Press TAB to resume or ENTER to return to menu", 20) / 2, screenHeight / 2 + 40, 20, LIGHTGRAY);
            }
        }

        // End drawing
        EndDrawing();
    }
    // Close and unload resources
    UnloadTexture(menuLogo);
    UnloadTexture(menuBg);
    if (bgLoaded) UnloadTexture(currentBg);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}