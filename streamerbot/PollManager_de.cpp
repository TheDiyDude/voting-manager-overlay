using System;
using System.IO;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using Newtonsoft.Json;

public class CPHInline
{
    private static string jsonFilePath = @"C:\Streaming\poll\poll_data.json";

    private static string title = "";
    private static List<string> options = new List<string>();
    private static Dictionary<int, int> votes = new Dictionary<int, int>();
    private static HashSet<string> votedUsers = new HashSet<string>();
    private static int totalTimeSec = 0;
    private static int remainingSec = 0;
    private static string status = "idle"; // idle, prepared, active, paused, ended
    private static string winner = "";
    private static CancellationTokenSource cts;

    public bool Execute()
    {
        string rawInput = args.ContainsKey("rawInput") ? args["rawInput"]?.ToString() ?? "" : "";
        string command = args.ContainsKey("command") ? args["command"]?.ToString() ?? "" : "";
        string message = args.ContainsKey("message") ? args["message"]?.ToString() ?? "" : "";
        string userId = args.ContainsKey("userId") ? args["userId"]?.ToString() ?? "" : "";

        if (string.IsNullOrEmpty(rawInput) && args.ContainsKey("input"))
        {
            rawInput = args["input"]?.ToString() ?? "";
        }

        if (command.Equals("!poll", StringComparison.OrdinalIgnoreCase))
        {
            ProcessManagerCommand(rawInput);
        }
        else
        {
            string inputToTest = !string.IsNullOrEmpty(command) ? command : message;
            ProcessVoteCommand(inputToTest, userId);
        }

        return true;
    }

    private void ProcessManagerCommand(string input)
    {
        input = input.Trim();

        if (input.Equals("start", StringComparison.OrdinalIgnoreCase))
        {
            if (status == "prepared")
            {
                status = "active";
                winner = "";
                StartTimerThread();
                SaveData();
                CPH.SendMessage(" Poll gestartet! Stimmt jetzt im Chat ab.");
            }
            return;
        }

        if (input.Equals("pause", StringComparison.OrdinalIgnoreCase))
        {
            if (status == "active")
            {
                cts?.Cancel();
                status = "paused";
                SaveData();
                CPH.SendMessage($" Poll pausiert. Verbleibend: {remainingSec / 60}m {remainingSec % 60}s");
            }
            return;
        }

        if (input.Equals("resume", StringComparison.OrdinalIgnoreCase))
        {
            if (status == "paused")
            {
                status = "active";
                StartTimerThread();
                SaveData();
                CPH.SendMessage(" Poll fortgesetzt!");
            }
            return;
        }

        if (input.Equals("reset", StringComparison.OrdinalIgnoreCase) || input.Equals("restart", StringComparison.OrdinalIgnoreCase))
        {
            if (options.Count > 0)
            {
                cts?.Cancel();
                votes.Clear();
                votedUsers.Clear();
                for (int i = 0; i < options.Count; i++) votes[i] = 0;
                
                remainingSec = totalTimeSec;
                status = "active";
                winner = "";
                StartTimerThread();
                SaveData();
                CPH.SendMessage($" Poll zurückgesetzt: \"{title}\" ({remainingSec / 60}m {remainingSec % 60}s)");
            }
            else
            {
                CPH.SendMessage(" Kein vorheriger Poll zum Zurücksetzen vorhanden.");
            }
            return;
        }

        if (input.Equals("end", StringComparison.OrdinalIgnoreCase))
        {
            EndPoll();
            return;
        }

        if (input.StartsWith("+") || input.StartsWith("-"))
        {
            if (status == "active" || status == "paused")
            {
                int change = ParseTimeInSeconds(input);
                remainingSec = Math.Max(0, remainingSec + change);
                SaveData();
                CPH.SendMessage($" Poll-Zeit angepasst. Verbleibend: {remainingSec / 60}m {remainingSec % 60}s");
            }
            return;
        }

        Match timeMatch = Regex.Match(input, @"^(\d+[ms])\s+");
        if (timeMatch.Success)
        {
            totalTimeSec = ParseTimeInSeconds(timeMatch.Groups[1].Value);
            remainingSec = totalTimeSec;

            string rest = input.Substring(timeMatch.Length);
            MatchCollection matches = Regex.Matches(rest, @"\""(.*?)\""");

            if (matches.Count >= 3 && matches.Count <= 11)
            {
                title = matches[0].Groups[1].Value;
                options.Clear();
                votes.Clear();
                votedUsers.Clear();
                winner = "";

                for (int i = 1; i < matches.Count; i++)
                {
                    options.Add(matches[i].Groups[1].Value);
                    votes[i - 1] = 0;
                }

                status = "prepared";
                SaveData();
                CPH.SendMessage($" Poll vorbereitet: \"{title}\" ({options.Count} Optionen). Starte mit '!poll start'");
            }
            else
            {
                CPH.SendMessage(" Syntax-Fehler: !poll 10m \"Titel\" \"Option 1\" \"Option 2\" (max 10 Optionen)");
            }
        }
    }

    private void ProcessVoteCommand(string voteInput, string userId)
    {
        if (status != "active") return;
        if (votedUsers.Contains(userId)) return;

        if (int.TryParse(voteInput.Trim(), out int choice))
        {
            if (choice >= 1 && choice <= options.Count)
            {
                int idx = choice - 1;
                votes[idx] = (votes.ContainsKey(idx) ? votes[idx] : 0) + 1;
                votedUsers.Add(userId);
                SaveData();
            }
        }
    }

    private void StartTimerThread()
    {
        cts?.Cancel();
        cts = new CancellationTokenSource();
        CancellationToken token = cts.Token;

        Task.Run(async () =>
        {
            while (remainingSec > 0 && status == "active")
            {
                try
                {
                    await Task.Delay(1000, token);
                    if (status == "active")
                    {
                        remainingSec--;
                        SaveData();
                    }
                }
                catch (TaskCanceledException)
                {
                    break;
                }
            }

            if (status == "active" && remainingSec <= 0)
            {
                EndPoll();
            }
        }, token);
    }

    private void EndPoll()
    {
        cts?.Cancel();
        status = "ended";

        int maxVotes = 0;
        int total = votedUsers.Count;

        if (total == 0)
        {
            winner = "Kein Ergebnis";
        }
        else
        {
            // 1. Höchste Stimmenzahl ermitteln
            for (int i = 0; i < options.Count; i++)
            {
                int currentVotes = votes.ContainsKey(i) ? votes[i] : 0;
                if (currentVotes > maxVotes)
                {
                    maxVotes = currentVotes;
                }
            }

            List<string> topOptions = new List<string>();

            for (int i = 0; i < options.Count; i++)
            {
                int currentVotes = votes.ContainsKey(i) ? votes[i] : 0;
                if (currentVotes == maxVotes && maxVotes > 0)
                {
                    topOptions.Add(options[i]);
                }
            }

            if (topOptions.Count > 1)
            {
                winner = "Unentschieden!";
            }
            else if (topOptions.Count == 1)
            {
                winner = topOptions[0];
            }
            else
            {
                winner = "Kein Ergebnis";
            }
        }

        int displayVotes = total == 0 ? 0 : maxVotes;

        SaveData();

        string chatAnnouncement = $" Poll Beendet! Ergebnis für \"{title}\": Gewinner ist '{winner}' mit {displayVotes} Stimmen (Gesamt: {total} Stimmen).";
        CPH.SendMessage(chatAnnouncement);
    }

    private int ParseTimeInSeconds(string timeStr)
    {
        bool isNegative = timeStr.StartsWith("-");
        timeStr = timeStr.Replace("+", "").Replace("-", "").Trim();

        int multiplier = 1;
        if (timeStr.EndsWith("m", StringComparison.OrdinalIgnoreCase))
        {
            multiplier = 60;
            timeStr = timeStr.Substring(0, timeStr.Length - 1);
        }
        else if (timeStr.EndsWith("s", StringComparison.OrdinalIgnoreCase))
        {
            multiplier = 1;
            timeStr = timeStr.Substring(0, timeStr.Length - 1);
        }

        if (int.TryParse(timeStr, out int val))
        {
            int total = val * multiplier;
            return isNegative ? -total : total;
        }
        return 0;
    }

    private void SaveData()
    {
        try
        {
            List<int> voteList = new List<int>();
            for (int i = 0; i < options.Count; i++) voteList.Add(votes.ContainsKey(i) ? votes[i] : 0);

            var payload = new
            {
                status = status,
                title = title,
                options = options,
                votes = voteList,
                totalVotes = votedUsers.Count,
                remainingSeconds = remainingSec,
                winner = winner
            };

            string json = JsonConvert.SerializeObject(payload);
            
            string dir = Path.GetDirectoryName(jsonFilePath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
            {
                Directory.CreateDirectory(dir);
            }

            File.WriteAllText(jsonFilePath, json);
        }
        catch { }
    }
}
