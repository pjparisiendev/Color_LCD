using System.Drawing.Drawing2D;

namespace ColorLcdSimulator;

internal sealed class SimulatorForm : Form
{
    private readonly RideModel ride = new();
    private readonly Dashboard preview;
    private readonly Label status = new() { AutoSize = true, ForeColor = Color.FromArgb(135, 163, 177) };
    private readonly System.Windows.Forms.Timer timer = new() { Interval = 100 };
    private bool settingsOpen;
    private bool settingsEditing;
    private int settingIndex;

    public SimulatorForm()
    {
        Text = "850C complete firmware simulation";
        BackColor = Color.Black;
        ForeColor = Color.White;
        MinimumSize = new Size(940, 780);
        Size = new Size(1120, 900);
        StartPosition = FormStartPosition.CenterScreen;
        KeyPreview = true;

        preview = new Dashboard(ride) { Dock = DockStyle.Fill };
        var controls = BuildControls();
        var toolbar = BuildToolbar();
        Controls.Add(preview);
        Controls.Add(controls);
        Controls.Add(toolbar);

        KeyDown += (_, e) =>
        {
            if (e.KeyCode is Keys.Right or Keys.P) NextPage();
            else if (e.KeyCode == Keys.Left) PreviousPage();
            else if (e.KeyCode == Keys.Up) ChangeAssist(1);
            else if (e.KeyCode == Keys.Down) ChangeAssist(-1);
            else if (e.KeyCode is Keys.Enter or Keys.Space) PressEnter();
            else if (e.KeyCode == Keys.C) ToggleSettings();
            else if (e.KeyCode == Keys.Escape) CloseSettings();
        };

        timer.Tick += (_, _) =>
        {
            ride.Tick(timer.Interval / 1000d);
            preview.Invalidate();
            status.Text = $"SIM {ride.LinkText}  •  {ride.TripKm:0.00} km  •  {ride.TripWh:0.0} Wh";
        };
        timer.Start();
    }

    protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
    {
        var key = keyData & Keys.KeyCode;
        if (key == Keys.Enter || key == Keys.Space) { PressEnter(); return true; }
        if (key == Keys.Up) { ChangeAssist(1); return true; }
        if (key == Keys.Down) { ChangeAssist(-1); return true; }
        if (key == Keys.Left) { PreviousPage(); return true; }
        if (key == Keys.Right || key == Keys.P) { NextPage(); return true; }
        if (key == Keys.C) { ToggleSettings(); return true; }
        if (key == Keys.Escape) { CloseSettings(); return true; }
        return base.ProcessCmdKey(ref msg, keyData);
    }

    private Control BuildToolbar()
    {
        var bar = new FlowLayoutPanel { Dock = DockStyle.Top, Height = 47, Padding = new Padding(10, 8, 6, 4), BackColor = Color.FromArgb(15, 15, 17) };
        bar.Controls.Add(Button("◀ Page", (_, _) => PreviousPage()));
        bar.Controls.Add(Button("Page ▶  [P]", (_, _) => NextPage()));
        bar.Controls.Add(Button("Assist +  [↑]", (_, _) => ChangeAssist(1)));
        bar.Controls.Add(Button("Assist −  [↓]", (_, _) => ChangeAssist(-1)));
        bar.Controls.Add(Button("Enter  [↵]", (_, _) => PressEnter()));
        bar.Controls.Add(Button("Settings  [C]", (_, _) => ToggleSettings()));
        bar.Controls.Add(Button("Reset trip", (_, _) => ride.ResetTrip()));
        bar.Controls.Add(status);
        return bar;
    }

    private Control BuildControls()
    {
        var panel = new FlowLayoutPanel { Dock = DockStyle.Right, Width = 300, FlowDirection = FlowDirection.TopDown, WrapContents = false, AutoScroll = true, Padding = new Padding(14), BackColor = Color.FromArgb(10, 10, 12) };
        panel.Controls.Add(Heading("LIVE RIDE CONTROLS  •  SCROLL ↓"));
        panel.Controls.Add(Slider("Speed", 0, 65, (int)ride.SpeedKph, v => ride.SpeedKph = v, "km/h"));
        panel.Controls.Add(Slider("Battery voltage", 390, 588, (int)(ride.Voltage * 10), v => ride.Voltage = v / 10d, "V", .1));
        panel.Controls.Add(Slider("Current", 0, 350, (int)(ride.CurrentA * 10), v => ride.CurrentA = v / 10d, "A", .1));
        panel.Controls.Add(Slider("Temperature", 10, 120, (int)ride.TemperatureC, v => ride.TemperatureC = v, "°C"));
        panel.Controls.Add(Slider("Throttle", 0, 100, ride.ThrottlePercent, v => ride.ThrottlePercent = v, "%"));
        panel.Controls.Add(Check("Brake active", v => ride.Brake = v));
        panel.Controls.Add(Check("Temperature available", v => ride.TemperatureAvailable = v, true));
        panel.Controls.Add(Check("Throttle available", v => ride.ThrottleAvailable = v, true));
        panel.Controls.Add(Check("Freeze UART telemetry", v => ride.FreezeUart = v));
        panel.Controls.Add(Heading("GRAPH PAGE"));
        for (var slot = 0; slot < 3; slot++)
        {
            var index = slot;
            var metric = new ComboBox { Width = 122, DropDownStyle = ComboBoxStyle.DropDownList };
            metric.Items.AddRange(["Power", "Speed", "Voltage", "Current", "SOC", "Temperature", "Wh/km"]);
            metric.SelectedIndex = ride.Graphs[index].Metric;
            metric.SelectedIndexChanged += (_, _) => ride.ConfigureGraph(index, metric.SelectedIndex, ride.Graphs[index].WindowIndex);
            var window = new ComboBox { Width = 122, DropDownStyle = ComboBoxStyle.DropDownList };
            window.Items.AddRange(["30 sec", "1 min", "5 min", "15 min", "30 min"]);
            window.SelectedIndex = ride.Graphs[index].WindowIndex;
            window.SelectedIndexChanged += (_, _) => ride.ConfigureGraph(index, ride.Graphs[index].Metric, window.SelectedIndex);
            var row = new FlowLayoutPanel { Width = 260, Height = 35, WrapContents = false };
            row.Controls.Add(metric); row.Controls.Add(window); panel.Controls.Add(row);
        }
        panel.Controls.Add(Heading("FAULT INJECTION"));
        panel.Controls.Add(Slider("Controller error", 0, 30, 0, v => ride.SetError((byte)v), ""));
        panel.Controls.Add(Button("Clear current error", (_, _) => ride.SetError(0)));
        panel.Controls.Add(Heading("BATTERY MODEL"));
        var battery = new ComboBox { Width = 250, DropDownStyle = ComboBoxStyle.DropDownList };
        battery.Items.AddRange(["36 V (42.0–30.0)", "48 V (54.6–39.0)", "52 V (58.8–42.0)"]);
        battery.SelectedIndex = ride.BatteryPreset;
        battery.SelectedIndexChanged += (_, _) => ride.SetBatteryPreset(battery.SelectedIndex);
        panel.Controls.Add(battery);
        panel.Controls.Add(new Label { Text = "The simulator computes SOC, watts, Wh, Wh/km, remaining Wh, range, peaks, UART age and error history live.", Width = 255, Height = 70, ForeColor = Color.FromArgb(135, 163, 177) });
        return panel;
    }

    private static Label Heading(string text) => new() { Text = text, Width = 255, Height = 30, Padding = new Padding(0, 8, 0, 0), Font = new Font("Segoe UI", 9, FontStyle.Bold), ForeColor = Color.FromArgb(38, 215, 183) };
    private static Button Button(string text, EventHandler action)
    {
        var b = new Button { Text = text, AutoSize = true, FlatStyle = FlatStyle.Flat, BackColor = Color.FromArgb(28, 28, 32), ForeColor = Color.White, Margin = new Padding(3) };
        b.FlatAppearance.BorderColor = Color.FromArgb(62, 62, 68); b.Click += action; return b;
    }
    private static CheckBox Check(string text, Action<bool> changed, bool initial = false)
    {
        var c = new CheckBox { Text = text, Width = 255, Checked = initial, ForeColor = Color.WhiteSmoke };
        c.CheckedChanged += (_, _) => changed(c.Checked); return c;
    }
    private static Control Slider(string name, int min, int max, int value, Action<int> changed, string unit, double scale = 1)
    {
        var box = new Panel { Width = 260, Height = 68 };
        var caption = new Label { Left = 0, Top = 0, Width = 255, ForeColor = Color.WhiteSmoke };
        var slider = new TrackBar { Left = 0, Top = 20, Width = 255, Minimum = min, Maximum = max, Value = Math.Clamp(value, min, max), TickStyle = TickStyle.None };
        void Update() { caption.Text = $"{name}: {slider.Value * scale:0.#} {unit}"; changed(slider.Value); }
        slider.ValueChanged += (_, _) => Update(); Update(); box.Controls.Add(caption); box.Controls.Add(slider); return box;
    }

    private void NextPage() { if (!settingsOpen) preview.CurrentPage = (preview.CurrentPage + 1) % 4; preview.Invalidate(); }
    private void PreviousPage() { if (!settingsOpen) preview.CurrentPage = (preview.CurrentPage + 3) % 4; preview.Invalidate(); }
    private void ChangeAssist(int delta)
    {
        if (settingsOpen)
        {
            if (settingsEditing) preview.AdjustSetting(settingIndex, delta);
            else settingIndex = Math.Clamp(settingIndex + delta, 0, Dashboard.SettingCount - 1);
        }
        else ride.Assist = Math.Clamp(ride.Assist + delta, 0, ride.AssistLevels);
        preview.SetSettings(settingsOpen, settingsEditing, settingIndex);
    }
    private void ToggleSettings() { settingsOpen = !settingsOpen; settingsEditing = false; preview.SetSettings(settingsOpen, false, settingIndex); }
    private void CloseSettings()
    {
        if (settingsEditing) settingsEditing = false;
        else settingsOpen = false;
        preview.SetSettings(settingsOpen, settingsEditing, settingIndex);
    }
    private void PressEnter()
    {
        if (!settingsOpen) return;
        if (settingIndex == Dashboard.SettingCount - 1 && !settingsEditing) { settingsOpen = false; settingsEditing = false; }
        else settingsEditing = !settingsEditing;
        preview.SetSettings(settingsOpen, settingsEditing, settingIndex);
    }
}

internal sealed class RideModel
{
    public double SpeedKph = 27.4, Voltage = 53.1, CurrentA = 12.8, TemperatureC = 61;
    public int Assist = 3, ThrottlePercent;
    public bool Brake, TemperatureAvailable = true, ThrottleAvailable = true, FreezeUart;
    public double TripKm, TripWh, PeakSpeed, PeakPower, PeakCurrent, PeakTemperature;
    // Confirmed pack rating is 48 V / 17.5 Ah / 840 Wh. Voltage endpoints are
    // intentionally editable because the exact Green Pedel pack curve is unverified.
    public double FullVoltage = 54.6, EmptyVoltage = 39, CapacityWh = 840;
    public double WheelInches = 27.5, TemperatureWarningC = 90;
    public int AssistLevels = 9, BrightnessPercent = 70, BatteryPreset = 1;
    public bool Metric = true;
    public byte ErrorCode;
    public readonly List<(byte Code, int Count)> Errors = [];
    public readonly GraphSlot[] Graphs = [new(0, 0), new(1, 2), new(2, 3)];
    private double uartAge, distanceCarry;
    public double PowerW => Voltage * CurrentA;
    public int Soc => (int)Math.Clamp(Math.Round((Voltage - EmptyVoltage) / (FullVoltage - EmptyVoltage) * 100), 0, 100);
    public double AverageWhKm => TripKm >= .5 ? TripWh / TripKm : double.NaN;
    public double InstantWhKm => TelemetryFresh && SpeedKph >= 5 && PowerW > 0 ? PowerW / SpeedKph : double.NaN;
    public double RemainingWh => CapacityWh * Soc / 100d;
    public double RangeKm => double.IsNaN(AverageWhKm) || AverageWhKm < 1 ? double.NaN : RemainingWh / AverageWhKm;
    public double InstantRangeKm => double.IsNaN(InstantWhKm) || InstantWhKm < 1 ? double.NaN : RemainingWh / InstantWhKm;
    public string LinkText => uartAge >= 2 ? "UART LOST" : uartAge >= .5 ? "UART STALE" : "UART OK";
    public bool TelemetryFresh => uartAge < .5;

    public void Tick(double seconds)
    {
        uartAge = FreezeUart ? uartAge + seconds : 0;
        if (!TelemetryFresh) return;
        var km = SpeedKph * seconds / 3600d;
        TripKm += km; distanceCarry += km;
        TripWh += PowerW * seconds / 3600d;
        PeakSpeed = Math.Max(PeakSpeed, SpeedKph); PeakPower = Math.Max(PeakPower, PowerW);
        PeakCurrent = Math.Max(PeakCurrent, CurrentA); if (TemperatureAvailable) PeakTemperature = Math.Max(PeakTemperature, TemperatureC);
        foreach (var graph in Graphs) graph.Tick(seconds, GraphValue(graph.Metric));
    }
    public void ResetTrip() { TripKm = TripWh = PeakSpeed = PeakPower = PeakCurrent = PeakTemperature = 0; Errors.Clear(); ErrorCode = 0; }
    public void SetBatteryPreset(int i) { BatteryPreset = Math.Clamp(i, 0, 2); (FullVoltage, EmptyVoltage, CapacityWh) = BatteryPreset switch { 0 => (42, 30, 500), 1 => (54.6, 39, 840), _ => (58.8, 42, 1000) }; }
    public void SetError(byte code)
    {
        ErrorCode = code; if (code == 0) return;
        var i = Errors.FindIndex(e => e.Code == code);
        if (i < 0) Errors.Insert(0, (code, 1)); else Errors[i] = (code, Errors[i].Count + 1);
        if (Errors.Count > 4) Errors.RemoveAt(4);
    }
    public double GraphValue(int metric) => metric switch { 1 => SpeedKph, 2 => Voltage, 3 => CurrentA, 4 => Soc, 5 => TemperatureC, 6 => double.IsNaN(InstantWhKm) ? 0 : InstantWhKm, _ => PowerW };
    public static string GraphName(int metric) => new[] { "POWER", "SPEED", "VOLTAGE", "CURRENT", "SOC", "TEMPERATURE", "WH/KM" }[metric];
    public static string GraphUnit(int metric) => new[] { "W", "KM/H", "V", "A", "%", "°C", "WH/KM" }[metric];
    public void ConfigureGraph(int slot, int metric, int window) { Graphs[slot] = new GraphSlot(metric, window); }
}

internal sealed class GraphSlot(int metric, int windowIndex)
{
    private static readonly double[] Windows = [30, 60, 300, 900, 1800];
    public int Metric { get; } = metric;
    public int WindowIndex { get; } = windowIndex;
    public Queue<double> History { get; } = new();
    private double elapsed, sum; private int samples;
    public void Tick(double seconds, double value)
    {
        elapsed += seconds; sum += value; samples++;
        var interval = Windows[WindowIndex] / 80d;
        if (elapsed < interval) return;
        History.Enqueue(sum / samples); while (History.Count > 80) History.Dequeue();
        elapsed = sum = 0; samples = 0;
    }
    public string WindowLabel => new[] { "30 SEC", "1 MIN", "5 MIN", "15 MIN", "30 MIN" }[WindowIndex];
}

internal sealed class Dashboard : Control
{
    public const int SettingCount = 10;
    private readonly RideModel r;
    private bool settings, editing; private int setting;
    public int CurrentPage;
    public Dashboard(RideModel model) { r = model; DoubleBuffered = true; BackColor = Color.Black; }
    public void SetSettings(bool open, bool isEditing, int index) { settings = open; editing = isEditing; setting = index; Invalidate(); }
    public void AdjustSetting(int index, int delta)
    {
        switch (index)
        {
            case 0: r.WheelInches = Math.Clamp(r.WheelInches + delta * .5, 20, 29); break;
            case 1: r.SetBatteryPreset((r.BatteryPreset + delta + 3) % 3); break;
            case 2: r.FullVoltage = Math.Clamp(r.FullVoltage + delta * .1, r.EmptyVoltage + 1, 65); break;
            case 3: r.EmptyVoltage = Math.Clamp(r.EmptyVoltage + delta * .1, 25, r.FullVoltage - 1); break;
            case 4: r.CapacityWh = Math.Clamp(r.CapacityWh + delta * 50, 100, 3000); break;
            case 5: r.AssistLevels = Math.Clamp(r.AssistLevels + delta, 3, 9); r.Assist = Math.Min(r.Assist, r.AssistLevels); break;
            case 6: r.TemperatureWarningC = Math.Clamp(r.TemperatureWarningC + delta, 40, 120); break;
            case 7: r.Metric = !r.Metric; break;
            case 8: r.BrightnessPercent = Math.Clamp(r.BrightnessPercent + delta * 10, 10, 100); break;
        }
        Invalidate();
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        base.OnPaint(e); e.Graphics.SmoothingMode = SmoothingMode.HighQuality;
        var scale = Math.Min((ClientSize.Width - 50f) / 320f, (ClientSize.Height - 50f) / 480f);
        e.Graphics.TranslateTransform((ClientSize.Width - 320 * scale) / 2, (ClientSize.Height - 480 * scale) / 2); e.Graphics.ScaleTransform(scale, scale);
        e.Graphics.FillRectangle(new SolidBrush(Color.Black), 0, 0, 320, 480);
        using (var panelEdge = new Pen(Color.FromArgb(48, 48, 52))) e.Graphics.DrawRectangle(panelEdge, 0, 0, 319, 479);
        if (settings) DrawSettings(e.Graphics); else { DrawHeader(e.Graphics); if (CurrentPage == 0) DrawRide(e.Graphics); else if (CurrentPage == 1) DrawBattery(e.Graphics); else if (CurrentPage == 2) DrawDiagnostics(e.Graphics); else DrawGraphs(e.Graphics); DrawFooter(e.Graphics); }
        e.Graphics.ResetTransform();
    }

    private void DrawHeader(Graphics g)
    {
        Card(g, 24, 6, 272, 38, 9, Surface);
        DrawLabel(g, $"{r.Voltage:0.0}V", 14, 34, 15, White, true); DrawLabel(g, $"BAT {r.Soc}%", 14, 105, 15, r.Soc < 20 ? Orange : Teal, true);
        DrawLabel(g, DateTime.Now.ToString("HH:mm"), 14, 246, 15, White, true);
        if (r.Soc > 0) Card(g, 174, 18, Math.Max(2, 54 * r.Soc / 100), 7, 3, r.Soc < 20 ? Orange : Teal);
    }
    private void DrawRide(Graphics g)
    {
        DrawLabel(g, "RIDING", 9, 24, 49, Muted, true); DrawSpeedGauge(g);
        DrawLabel(g, r.TelemetryFresh ? r.SpeedKph.ToString("0.0") : "--", 43, 160, 105, White, true, true);
        DrawLabel(g, "KM/H", 10, 160, 91, Muted, true, true);
        DrawLabel(g, Fresh($"{r.PowerW:0} W"), 15, 160, 184, White, true, true);
        DrawLabel(g, "ASSIST", 8, 160, 217, Muted, true, true); DrawLabel(g, r.Assist.ToString(), 27, 160, 229, Teal, true, true);
        CompactMetric(g, 24, 306, 84, "TRIP", $"{r.TripKm:0.0} km", White); CompactMetric(g, 118, 306, 84, "RANGE", Value(r.RangeKm, "0", " km"), Teal); CompactMetric(g, 212, 306, 84, "CURRENT", Fresh($"{r.CurrentA:0.0} A"), White);
        Card(g, 24, 358, 272, 52, 8, Surface); DrawLabel(g, "AVG", 8, 34, 366, Muted, true); DrawLabel(g, Value(r.AverageWhKm, "0.0", " Wh/km"), 13, 34, 383, EfficiencyColor(r.AverageWhKm), true); DrawLabel(g, "INSTANT", 8, 170, 366, Muted, true); DrawLabel(g, Value(r.InstantWhKm, "0.0", " Wh/km"), 13, 170, 383, EfficiencyColor(r.InstantWhKm), true);
        DrawLabel(g, r.Brake ? "● BRAKE" : r.LinkText, 10, 286, 423, r.Brake || !r.TelemetryFresh ? Orange : Teal, true, true);
    }
    private void DrawSpeedGauge(Graphics g)
    {
        var rect = new RectangleF(45, 59, 230, 230);
        using var track = new Pen(Color.FromArgb(49, 49, 55), 6) { StartCap = LineCap.Round, EndCap = LineCap.Round };
        using var green = new Pen(Teal, 6) { StartCap = LineCap.Round, EndCap = LineCap.Round };
        using var amber = new Pen(Amber, 6); using var red = new Pen(Red, 6);
        g.DrawEllipse(track, rect); g.DrawArc(green, rect, 180, 105); g.DrawArc(amber, rect, 285, 45); g.DrawArc(red, rect, 330, 30); g.DrawArc(amber, rect, 0, 180);
        var center = new PointF(160, 174); const float outer = 111, inner = 102;
        using var tick = new Pen(Color.FromArgb(180, 182, 190), 1);
        for (var i = 0; i <= 12; i++) { var a = (180 + i * 180f / 12) * MathF.PI / 180; g.DrawLine(tick, center.X + MathF.Cos(a) * inner, center.Y + MathF.Sin(a) * inner, center.X + MathF.Cos(a) * outer, center.Y + MathF.Sin(a) * outer); }
        for (var i = 0; i <= 6; i++) { var a = (180 + i * 30) * MathF.PI / 180; var x = center.X + MathF.Cos(a) * 86; var y = center.Y + MathF.Sin(a) * 86; DrawLabel(g, (i * 10).ToString(), 8, x, y - 4, White, true, true); }
        for (var i = 0; i <= 4; i++) { var a = (180 - i * 45f) * MathF.PI / 180; var x = center.X + MathF.Cos(a) * 91; var y = center.Y + MathF.Sin(a) * 91; DrawLabel(g, (i * 250).ToString(), 7, x, y - 3, Muted, false, true); }
        var angle = (180 + Math.Clamp((float)r.SpeedKph / 60, 0, 1) * 180) * MathF.PI / 180;
        using var marker = new Pen(White, 3) { StartCap = LineCap.Round, EndCap = LineCap.Round };
        g.DrawLine(marker, center.X + MathF.Cos(angle) * 93, center.Y + MathF.Sin(angle) * 93, center.X + MathF.Cos(angle) * 113, center.Y + MathF.Sin(angle) * 113);
        var powerAngle = (180 - Math.Clamp((float)r.PowerW / 1000, 0, 1) * 180) * MathF.PI / 180;
        using var powerMarker = new Pen(Amber, 3); g.DrawLine(powerMarker, center.X + MathF.Cos(powerAngle) * 93, center.Y + MathF.Sin(powerAngle) * 93, center.X + MathF.Cos(powerAngle) * 113, center.Y + MathF.Sin(powerAngle) * 113);
        using var divider = new Pen(Color.FromArgb(90, 90, 96)); g.DrawLine(divider, 91, 207, 229, 207);
    }
    private void DrawBattery(Graphics g)
    {
        DrawLabel(g, "BATTERY / EFFICIENCY", 12, 12, 57, Teal, true);
        Metric(g, 8, 82, 148, 68, "VOLTAGE / SOC", $"{r.Voltage:0.0} V  {r.Soc}%", White); Metric(g, 164, 82, 148, 68, "LIVE LOAD", Fresh($"{r.CurrentA:0.0} A"), White);
        Metric(g, 8, 158, 148, 68, "POWER", Fresh($"{r.PowerW:0} W"), White); Metric(g, 164, 158, 148, 68, "TRIP ENERGY", $"{r.TripWh:0.0} Wh", White);
        Metric(g, 8, 234, 148, 68, "AVG EFFICIENCY", Value(r.AverageWhKm, "0.0", " Wh/km"), Teal); Metric(g, 164, 234, 148, 68, "REMAINING", $"{r.RemainingWh:0} Wh", White);
        Metric(g, 8, 310, 148, 68, "PEAK CURRENT", $"{r.PeakCurrent:0.0} A", White); Metric(g, 164, 310, 148, 68, "PEAK POWER", $"{r.PeakPower:0} W", White);
        Metric(g, 8, 386, 148, 52, "AVG RANGE", Value(r.RangeKm, "0.0", " km"), Teal); Metric(g, 164, 386, 148, 52, "INSTANT RANGE", Value(r.InstantRangeKm, "0.0", " km"), White);
    }
    private void DrawDiagnostics(Graphics g)
    {
        DrawLabel(g, "MOTOR / DIAGNOSTICS", 12, 12, 57, Teal, true);
        Card(g, 8, 82, 304, 55, 8, Surface); DrawLabel(g, r.LinkText, 18, 20, 97, r.TelemetryFresh ? Teal : Red, true); DrawLabel(g, r.TelemetryFresh ? "receiving telemetry" : "values hidden", 9, 297, 105, Muted, false, true);
        Metric(g, 8, 146, 148, 65, "TEMPERATURE", r.TemperatureAvailable ? Fresh($"{r.TemperatureC:0} °C") : "N/A", r.TemperatureC >= 90 ? Orange : White);
        Metric(g, 164, 146, 148, 65, "PEAK TEMP", r.TemperatureAvailable ? $"{r.PeakTemperature:0} °C" : "N/A", White);
        Metric(g, 8, 219, 96, 62, "BRAKE", Fresh(r.Brake ? "ACTIVE" : "OFF"), r.Brake ? Orange : White); Metric(g, 112, 219, 96, 62, "THROTTLE", r.ThrottleAvailable ? Fresh($"{r.ThrottlePercent}%") : "N/A", White); Metric(g, 216, 219, 96, 62, "PAS", Fresh(r.Assist.ToString()), Teal);
        Metric(g, 8, 289, 148, 62, "VOLTAGE", Fresh($"{r.Voltage:0.0} V"), White); Metric(g, 164, 289, 148, 62, "CURRENT", Fresh($"{r.CurrentA:0.0} A"), White);
        Card(g, 8, 359, 304, 78, 8, Surface); DrawLabel(g, $"CURRENT ERROR   {(r.ErrorCode == 0 ? "NONE" : $"E{r.ErrorCode:00}")}", 11, 18, 372, r.ErrorCode == 0 ? White : Red, true);
        var history = r.Errors.Count == 0 ? "History: none" : "History: " + string.Join("  ", r.Errors.Select(x => $"E{x.Code:00}×{x.Count}")); DrawLabel(g, history, 10, 18, 405, Muted, true);
    }
    private void DrawGraphs(Graphics g)
    {
        DrawLabel(g, "GRAPHS", 12, 12, 57, Teal, true);
        for (var slot = 0; slot < 3; slot++)
        {
            var graph = r.Graphs[slot]; var top = 82 + slot * 116;
            Card(g, 8, top, 304, 108, 8, Surface);
            DrawLabel(g, $"{RideModel.GraphName(graph.Metric)}  •  {graph.WindowLabel}", 9, 16, top + 8, Muted, true);
            var values = graph.History.ToArray();
            if (values.Length > 1)
            {
                var min = values.Min(); var max = Math.Max(min + .1, values.Max());
                var points = values.Select((v, i) => new PointF(16 + i * 288f / 79f, top + 91 - (float)((v - min) / (max - min) * 62))).ToArray();
                using var line = new Pen(slot == 0 ? Teal : slot == 1 ? Color.DeepSkyBlue : Color.Orange, 2); g.DrawLines(line, points);
            }
            DrawLabel(g, $"{r.GraphValue(graph.Metric):0.0} {RideModel.GraphUnit(graph.Metric)}", 11, 296, top + 8, White, true, true);
        }
    }
    private void DrawSettings(Graphics g)
    {
        string[] items = ["Wheel size", "Battery preset", "Full voltage", "Empty voltage", "Capacity", "Assist levels", "Temperature warning", "Units", "Display brightness", "Exit"];
        string[] values = [$"{r.WheelInches:0.#} in", new[] { "36 V", "48 V", "52 V" }[r.BatteryPreset], $"{r.FullVoltage:0.0} V", $"{r.EmptyVoltage:0.0} V", $"{r.CapacityWh:0} Wh", r.AssistLevels.ToString(), $"{r.TemperatureWarningC:0} °C", r.Metric ? "Metric" : "Imperial", $"{r.BrightnessPercent}%", ""];
        DrawLabel(g, "FIRMWARE SETTINGS", 16, 15, 13, Teal, true);
        for (var i = 0; i < items.Length; i++)
        {
            var y = 47 + i * 39;
            if (i == setting) Card(g, 7, y - 3, 306, 34, 7, editing ? Color.FromArgb(25, 92, 82) : Color.FromArgb(22, 74, 88));
            DrawLabel(g, (i == setting ? (editing ? "◆ " : "› ") : "  ") + items[i], 11, 13, y + 5, i == setting ? White : Muted, true);
            DrawLabel(g, values[i], 11, 250, y + 5, i == setting && editing ? Color.Yellow : i == setting ? Teal : White, false, true);
        }
        DrawLabel(g, editing ? "↑↓ CHANGE   ENTER SAVE   ESC BACK" : "↑↓ SELECT   ENTER EDIT   ESC CLOSE", 9, 160, 451, Muted, true, true);
    }
    private void DrawFooter(Graphics g) { DrawLabel(g, $"{CurrentPage + 1}/4   P: NEXT PAGE   C: SETTINGS", 9, 160, 457, Muted, true, true); }
    private string Fresh(string value) => r.TelemetryFresh ? value : "--";
    private static string Value(double value, string format, string suffix) => double.IsNaN(value) ? "--" : value.ToString(format) + suffix;
    private static void Metric(Graphics g, int x, int y, int w, int h, string label, string value, Color color) { Card(g, x, y, w, h, 8, SurfaceRaised); DrawLabel(g, label, 9, x + 10, y + 9, Muted, true); DrawLabel(g, value, 16, x + w / 2, y + 31, color, true, true); }
    private static void CompactMetric(Graphics g, int x, int y, int w, string label, string value, Color color) { Card(g, x, y, w, 42, 7, SurfaceRaised); DrawLabel(g, label, 8, x + 8, y + 6, Muted, true); DrawLabel(g, value, 14, x + w / 2, y + 20, color, true, true); }
    private static void DrawLabel(Graphics g, string text, float size, float x, float y, Color color, bool bold = false, bool center = false) { using var f = new Font(FontFamily.GenericSansSerif, size, bold ? FontStyle.Bold : FontStyle.Regular, GraphicsUnit.Pixel); using var b = new SolidBrush(color); using var sf = new StringFormat { Alignment = center ? StringAlignment.Center : StringAlignment.Near }; g.DrawString(text, f, b, new RectangleF(center ? 0 : x, y, center ? x * 2 : 300, size + 8), sf); }
    private static void Card(Graphics g, float x, float y, float w, float h, float radius, Color color) { using var p = new GraphicsPath(); float d = radius * 2; p.AddArc(x, y, d, d, 180, 90); p.AddArc(x + w - d, y, d, d, 270, 90); p.AddArc(x + w - d, y + h - d, d, d, 0, 90); p.AddArc(x, y + h - d, d, d, 90, 90); p.CloseFigure(); using var b = new SolidBrush(color); g.FillPath(b, p); }
    private static Color EfficiencyColor(double value) => double.IsNaN(value) ? Muted : value <= 15 ? Teal : value <= 25 ? Amber : Red;
    private static readonly Color Teal = Color.FromArgb(48, 209, 126), Red = Color.FromArgb(255, 77, 94), Amber = Color.FromArgb(255, 184, 77), White = Color.FromArgb(245, 247, 250), Muted = Color.FromArgb(145, 148, 158), Orange = Color.FromArgb(255, 105, 72), Surface = Color.FromArgb(17, 17, 20), SurfaceRaised = Color.FromArgb(25, 25, 29);
}
