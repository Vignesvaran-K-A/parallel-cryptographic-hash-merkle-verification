# /*
#  * ===========================================================================
#  * Project Title      : Parallel Cryptographic Hash & Merkle Tree
#  *                      Verification Engine (QNX Neutrino RTOS)
#  * File               : ground_station.py (GUI)
#  * Brief Note on Project :
#  *     Host dashboard for generating a 64 KB firmware payload, intentionally
#  *     marking selected blocks, deploying the image to the QNX target over SSH,
#  *     launching the client, and displaying the returned integrity result.
#  * Team Members       : K A Vignesvaran   - CB.EN.U4ELC23021
#  *                      Kiruthik Pranav T A - CB.EN.U4ELC23023
#  *                      Vidhyarth S E     - CB.EN.U4ELC23057
#  * Course             : 26ELC331 Real Time Operating System using QNX
#  * Date of Evaluation : October 2026
#  * ===========================================================================
import os
import json
import threading
import tkinter as tk
from tkinter import ttk
import paramiko

# SSH configuration is read from environment variables.
# Never put passwords or private keys directly in source code.
PI_IP = os.environ.get("QNX_HOST", "10.0.0.1")
PI_USER = os.environ.get("QNX_USER", "root")
PI_PASS = os.environ.get("QNX_PASSWORD")

BLOCK_SIZE = 4096
ANIM_DELAY_MS = 50

# Log prefix -> console tag (UI/UX: color-coded telemetry)
LOG_TAGS = {
    "[SSH]": "cyan",
    "[SFTP]": "cyan",
    "[IPC]": "cyan",
    "[HEX]": "cyan",
    "[WARN]": "red",
    "[FATAL ERROR]": "red",
    "[SYSTEM]": "yellow",
    "[QNX RESPONSE]": "green",
}


class MerkleDashboard(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Zero-Touch OTA Ground Station")
        self.geometry("650x760")
        self.configure(bg="#0F172A")

        self.checkbox_vars = []

        # --- Section 1: Payload Configuration ---
        tk.Label(
            self,
            text="1. Select Blocks to Intentionally Corrupt",
            fg="white",
            bg="#0F172A",
            font=("Arial", 12, "bold"),
        ).pack(pady=(15, 5))

        grid_frame = tk.Frame(self, bg="#0F172A")
        grid_frame.pack(pady=5)

        for i in range(16):
            var = tk.IntVar()
            cb = tk.Checkbutton(
                grid_frame,
                text=f"Block {i}",
                variable=var,
                fg="black",
                bg="#CBD5E1",
                selectcolor="#DC2626",
                font=("Arial", 9, "bold"),
                width=8,
                indicatoron=0,
            )
            row, col = divmod(i, 4)
            cb.grid(row=row, column=col, padx=5, pady=5)
            self.checkbox_vars.append(var)

        self.deploy_btn = tk.Button(
            self,
            text="🚀 ONE-CLICK DEPLOY & VERIFY",
            command=self.start_deployment_thread,
            bg="#0284C7",
            fg="white",
            font=("Arial", 12, "bold"),
            width=35,
            height=2,
        )
        self.deploy_btn.pack(pady=(15, 8))

        # --- Deployment progress bar ---
        style = ttk.Style(self)
        style.theme_use("clam")
        style.configure(
            "Ota.Horizontal.TProgressbar",
            troughcolor="#1E293B",
            background="#0284C7",
            bordercolor="#0F172A",
            lightcolor="#0284C7",
            darkcolor="#0284C7",
        )
        self.progress = ttk.Progressbar(
            self,
            style="Ota.Horizontal.TProgressbar",
            orient="horizontal",
            length=400,
            mode="determinate",
            maximum=100,
        )
        self.progress.pack(pady=(0, 8))

        # --- Section 2: Integrity Visualizer ---
        self.status_lbl = tk.Label(
            self,
            text="System Standby...",
            fg="#94A3B8",
            bg="#0F172A",
            font=("Arial", 14, "bold"),
        )
        self.status_lbl.pack(pady=5)

        self.canvas = tk.Canvas(
            self,
            width=400,
            height=100,
            bg="#0F172A",
            highlightthickness=0,
        )
        self.canvas.pack()
        self.render_empty_grid()

        # --- Section 3: Live Telemetry Console ---
        tk.Label(
            self,
            text="Live Remote Telemetry",
            fg="white",
            bg="#0F172A",
            font=("Arial", 10, "bold"),
        ).pack(pady=(15, 0))

        self.console = tk.Text(
            self,
            height=12,
            width=70,
            bg="#1E293B",
            fg="#10B981",
            font=("Courier", 9),
            padx=10,
            pady=10,
            state=tk.DISABLED,
        )
        self.console.pack(pady=5)

        self.console.tag_config("cyan", foreground="#22D3EE")
        self.console.tag_config("red", foreground="#EF4444")
        self.console.tag_config("yellow", foreground="#FDE047")
        self.console.tag_config("green", foreground="#10B981")

    # ---------- Helpers ----------
    def set_progress(self, value):
        """Thread-safe progress bar update."""
        self.after(0, lambda: self.progress.config(value=value))

    def _insert(self, text, tag="green"):
        """Main-thread only: append text to the console with a color tag."""
        self.console.config(state=tk.NORMAL)
        self.console.insert(tk.END, text, tag)
        self.console.see(tk.END)
        self.console.config(state=tk.DISABLED)

    def log(self, message, color=None):
        """Append telemetry safely from any thread."""
        tag = color or "green"
        for prefix, mapped_tag in LOG_TAGS.items():
            if message.startswith(prefix):
                tag = mapped_tag
                break
        self.after(0, lambda: self._insert(message + "\n", tag))

    def render_empty_grid(self):
        self.canvas.delete("all")
        size, pad = 20, 5
        start_x = 200 - (8 * size + 7 * pad) / 2
        for i in range(16):
            x = start_x + (i % 8) * (size + pad)
            y = 10 + (i // 8) * (size + pad)
            self.canvas.create_rectangle(
                x, y, x + size, y + size, fill="#475569", outline="white"
            )

    def start_deployment_thread(self):
        self.deploy_btn.config(state=tk.DISABLED, bg="#64748B")
        self.console.config(state=tk.NORMAL)
        self.console.delete("1.0", tk.END)
        self.console.config(state=tk.DISABLED)
        self.progress.config(value=0)
        self.render_empty_grid()
        self.status_lbl.config(text="Deploying Firmware...", fg="#FDE047")
        threading.Thread(target=self.execute_deployment, daemon=True).start()

    def execute_deployment(self):
        try:
            if not PI_PASS:
                raise RuntimeError(
                    "Set the QNX_PASSWORD environment variable before launching the GUI."
                )

            # 1. Generate payload (64 KB, split into sixteen 4 KB blocks).
            self.log("[SYSTEM] Generating 64KB firmware payload...")
            payload = bytearray([0x01] * 65536)
            for i, var in enumerate(self.checkbox_vars):
                if var.get() == 1:
                    payload[i * BLOCK_SIZE] = 0xFF
                    self.log(f"[WARN] Injected anomaly into Block {i}")

            with open("update.bin", "wb") as firmware_file:
                firmware_file.write(payload)
            self.set_progress(20)

            # 2. SSH connection.
            self.log(f"[SSH] Establishing secure connection to {PI_IP}...")
            ssh = paramiko.SSHClient()
            ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            ssh.connect(
                PI_IP,
                username=PI_USER,
                password=PI_PASS,
                timeout=5,
                look_for_keys=False,
                allow_agent=False,
            )
            self.set_progress(35)

            try:
                # 3. Stream the binary directly to the target RAM-disk.
                self.log("[SSH] Piping update.bin directly into QNX RAM-disk...")
                stdin, stdout, stderr = ssh.exec_command("cat > /tmp/update.bin")
                stdin.write(payload)
                stdin.channel.shutdown_write()
                transfer_error = stderr.read().decode("utf-8", errors="replace").strip()
                if transfer_error:
                    self.log(f"[WARN] Transfer message: {transfer_error}")
                self.set_progress(50)

                # 4. Launch the QNX client; the QNX server must already be running.
                self.log("[IPC] Triggering QNX verification client...")
                self.set_progress(65)
                stdin, stdout, stderr = ssh.exec_command(
                    "cd /tmp && chmod +x merkle-client && ./merkle-client update.bin"
                )
                qnx_output = stdout.read().decode("utf-8").strip()
                qnx_error = stderr.read().decode("utf-8", errors="replace").strip()
                if qnx_error:
                    self.log(f"[WARN] QNX stderr: {qnx_error}")
                self.log(f"[QNX RESPONSE] {qnx_output}")
                self.set_progress(80)
            finally:
                ssh.close()

            # 5. Parse and render the JSON result.
            if not qnx_output:
                raise RuntimeError(
                    "QNX client returned no result. Check that merkle-server is running "
                    "and that the client completed successfully."
                )

            data = json.loads(qnx_output)
            self.after(0, self.render_results, data)

        except Exception as exc:
            self.log(f"[FATAL ERROR] {exc}")
            self.after(
                0,
                lambda: self.status_lbl.config(
                    text="DEPLOYMENT FAILED", fg="#EF4444"
                ),
            )
            self.after(
                0, lambda: self.deploy_btn.config(state=tk.NORMAL, bg="#0284C7")
            )

    def render_results(self, data):
        self.canvas.delete("all")
        self.render_empty_grid()
        mask = data.get("mask", 0)
        status = data.get("status", "UNKNOWN")

        status_color = "#10B981" if status == "VALID" else "#EF4444"
        self.status_lbl.config(text=f"Integrity Status: {status}", fg=status_color)
        self.animate_block(0, mask)

    def animate_block(self, i, mask):
        if i >= 16:
            self.finish_render(mask)
            return

        size, pad = 20, 5
        start_x = 200 - (8 * size + 7 * pad) / 2
        x = start_x + (i % 8) * (size + pad)
        y = 10 + (i // 8) * (size + pad)
        is_corrupt = (mask & (1 << i)) != 0
        color = "#EF4444" if is_corrupt else "#10B981"
        self.canvas.create_rectangle(
            x, y, x + size, y + size, fill=color, outline="white"
        )
        self.canvas.create_text(
            x + size / 2,
            y + size / 2,
            text=str(i),
            fill="white",
            font=("Arial", 7, "bold"),
        )

        self.progress.config(value=80 + (i + 1) * 20 / 16)
        self.after(ANIM_DELAY_MS, self.animate_block, i + 1, mask)

    def finish_render(self, mask):
        self.progress.config(value=100)
        self._insert("[SYSTEM] Dashboard visualizer updated successfully.\n", "yellow")
        for i in range(16):
            if mask & (1 << i):
                self.show_hex_preview(i)
        self.deploy_btn.config(state=tk.NORMAL, bg="#0284C7")

    def show_hex_preview(self, block):
        try:
            with open("update.bin", "rb") as firmware_file:
                firmware_file.seek(block * BLOCK_SIZE)
                chunk = firmware_file.read(16)
        except OSError as exc:
            self._insert(f"[FATAL ERROR] Hex preview failed: {exc}\n", "red")
            return

        self._insert(f"[HEX] Block {block} @ 0x{block * BLOCK_SIZE:05X}: ", "cyan")
        for byte in chunk:
            self._insert(f"{byte:02X} ", "red" if byte == 0xFF else "green")
        self._insert("\n", "green")


if __name__ == "__main__":
    app = MerkleDashboard()
    app.mainloop()
