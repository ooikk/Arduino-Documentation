
# ⚙️ Remove Software Write Protection on Windows   
If the physical switch isn't the problem, the write protection might be set in software.
Clear Read-Only Attributes with DiskPart
Press Win + R, type cmd, and press Ctrl + Shift + Enter to open an Administrator Command Prompt.
Type diskpart and press Enter.
Type list disk and press Enter. Identify your SD card by its size.
Type select disk X (replace X with your SD card's disk number) and press Enter.
Type attributes disk clear readonly and press Enter.
Wait for the confirmation, then type exit to close DiskPart.
Modify the Windows Registry (Alternative)
If DiskPart doesn't work, you can remove the write protection through the Registry Editor.
Press Win + R, type regedit, and press Enter.
Navigate to: HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\StorageDevicePolicies
If you don't see StorageDevicePolicies, you may need to create it (see note below).
Double‑click the WriteProtect DWORD, set its Value data to 0, and click OK.
Close the Registry Editor and restart your computer.
Note: If the StorageDevicePolicies key doesn't exist, you can create it. Right‑click in the right pane, select New > Key, and name it exactly. Then inside it, create a new DWORD (32‑bit) Value named WriteProtect.
