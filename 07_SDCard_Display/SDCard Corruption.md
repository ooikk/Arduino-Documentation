
# ⚙️ Remove Software Write Protection on Windows   
If the physical switch isn't the problem, the write protection might be set in software.    

## Clear Read-Only Attributes with DiskPart:    
1. Press Win + R, type cmd, and press Ctrl + Shift + Enter to open an Administrator Command Prompt.
2. Type diskpart and press Enter.
3. Type list disk and press Enter. Identify your SD card by its size.
4. Type select disk X (replace X with your SD card's disk number) and press Enter.
5. Type attributes disk clear readonly and press Enter.
6. Wait for the confirmation, then type exit to close DiskPart.

## Modify the Windows Registry (Alternative)
1. If DiskPart doesn't work, you can remove the write protection through the Registry Editor.
2. Press Win + R, type regedit, and press Enter.
3. Navigate to: HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\StorageDevicePolicies
4. If you don't see StorageDevicePolicies, you may need to create it (see note below).
5. Double‑click the WriteProtect DWORD, set its Value data to 0, and click OK.
6. Close the Registry Editor and restart your computer.    
*Note:* If the StorageDevicePolicies key doesn't exist, you can create it. Right‑click in the right pane, select New > Key, and name it exactly. Then inside it, create a new DWORD (32‑bit) Value named WriteProtect.
