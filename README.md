# Streamer.bot Retro Poll Overlay

A custom, interactive poll system designed for **Streamer.bot** (v1.0.7+) and **OBS Studio** with a cyberpunk/retro synthwave aesthetic.

## Features
- **Flexible Polls**: Highly customizable with up to 10 voting options.
- **Full Chat Control**: Create, start, pause, resume, reset, and adjust remaining time using simple chat commands.
- **Smart Chat Voting**: Viewers vote by sending option numbers (`1`, `2`, etc.). Out-of-bounds numbers are ignored, and double-voting is prevented.
- **Smart Winner Handling**: Built-in tie detection ("Unentschieden!") and fallback handling for 0-vote scenarios ("Kein Ergebnis").
- **Animated Ticker**: Dynamic marquee ticker at the bottom explaining how to submit votes in chat.

## Commands (Moderator / Broadcaster)

| Command | Description |
| :--- | :--- |
| `!poll 10m "Title" "Opt 1" "Opt 2"` | Prepares a poll with duration (e.g., 10m, 90s) |
| `!poll start` | Starts the prepared poll |
| `!poll pause` | Pauses the timer |
| `!poll resume` | Resumes the timer |
| `!poll restart` | Resets and restarts the poll with all existing options |
| `!poll +2m` / `!poll -1m` | Adjusts remaining time during an active poll |
| `!poll end` | Ends the poll early and announces the final result |

## Setup Instructions

### 1. File Placement
1. Create the directory `C:\StreamOverlay\`.
2. Save the `overlay/poll_overlay.html` file into `C:\StreamOverlay\poll_overlay.html`.

### 2. Streamer.bot Setup
1. Create a new Action named `Poll Core Manager`.
2. Add a C# Subaction: `Core -> C# -> Execute C# Code`.
3. Paste the code from `streamerbot/PollManager.cs` into the subaction and compile it.
4. Create two triggers under **Commands**:
   - `!poll` $\rightarrow$ Link trigger to the `Poll Core Manager` action (`Command Triggered: !poll`).
   - `Poll Vote` with lines `1` through `10` $\rightarrow$ Link trigger to the same action.

### 3. OBS Studio
1. Add a new **Browser Source**.
2. Check **Local file** and select `C:\StreamOverlay\poll_overlay.html`.
3. Set the dimensions to **600 x 800** px.

## License
MIT License - See [LICENSE](LICENSE) for details.

## Contact

**Marc-Oliver Blumenauer**  
Email: [marc@l3c.de](mailto:marc@l3c.de)

If you want to get me a cup of coffee I appreciate: 

[![Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/randvieh)

	
