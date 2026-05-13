import os

def convert_chart_to_txt(input_chart, output_txt):
    resolution = 192  # Default standard resolution
    bpm = 120.0       # Default fallback
    notes = []
    
    in_expert_section = False
    
    try:
        with open(input_chart, 'r') as f:
            lines = f.readlines()
            
        for line in lines:
            line = line.strip()
            
            # 1. Grab the Resolution from the [Song] block
            if line.startswith("Resolution ="):
                resolution = int(line.split("=")[1].strip())
                
            # 2. Grab the BPM from the [SyncTrack] block
            # Chart files store BPM as actual_bpm * 1000 (e.g., 160000 = 160.0 BPM)
            elif "= B" in line:
                # We only grab the first BPM event to keep it simple
                parts = line.split("= B")
                if len(parts) == 2:
                    bpm = float(parts[1].strip()) / 1000.0
                    
            # 3. Detect when we enter and exit the Expert notes section
            elif line == "[ExpertSingle]":
                in_expert_section = True
            elif line == "}" and in_expert_section:
                in_expert_section = False
                
            # 4. Extract the notes
            elif in_expert_section and "= N" in line:
                # Format looks like: "192 = N 0 0" (Tick = Note Lane Length)
                left_side, right_side = line.split("=")
                tick = int(left_side.strip())
                
                note_data = right_side.strip().split()
                if len(note_data) >= 2 and note_data[0] == "N":
                    lane = int(note_data[1])
                    
                    # We only want standard game lanes (0, 1, 2, 3)
                    # Ignore special Moonscraper flags like 5 (Forced) or 6 (Tap)
                    if lane in [0, 1, 2, 3]:
                        notes.append((tick, lane))
                        
    except FileNotFoundError:
        print(f"Error: Could not find {input_chart}")
        return

    # 5. Convert Ticks to Seconds and save to the .txt file
    with open(output_txt, 'w') as f:
        for tick, lane in notes:
            # The Magic Formula: Seconds = (Ticks / Resolution) * (60 / BPM)
            beats = tick / resolution
            time_sec = beats * (60.0 / bpm)
            
            # Format to 3 decimal places to match your C code's printf
            f.write(f"{time_sec:.3f} {lane}\n")

    print(f"Success! Converted {len(notes)} notes.")
    print(f"Detected Song Settings: {bpm} BPM | {resolution} Resolution.")
    print(f"Saved to {output_txt}!")

# --- Script Execution ---
if __name__ == "__main__":
    # Ensure these match the names of your files!
    INPUT_FILE = "cyberpunk2077.chart"
    OUTPUT_FILE = "cyberpunk2077.txt"

    print(f"Translating {INPUT_FILE} to C-readable text file...")
    convert_chart_to_txt(INPUT_FILE, OUTPUT_FILE)