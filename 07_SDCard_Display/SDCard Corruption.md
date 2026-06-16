
# ⚙️ Remove Software Write Protection on Windows   
If the physical switch isn't the problem, the write protection might be set in software.    

## Clear Read-Only Attributes with DiskPart:    
1. Press Win + R, type **cmd**, and press Ctrl + Shift + Enter to open an Administrator Command Prompt.
2. Type **diskpart** and press Enter.
3. Type **list disk** and press Enter. Identify your SD card by its size.
4. Type **select disk X** (replace X with your SD card's disk number) and press Enter.
5. Type **attributes disk clear readonly** and press Enter.
6. Wait for the confirmation, then type **exit** to close DiskPart.

## Modify the Windows Registry (Alternative)
1. If **DiskPart** doesn't work, you can remove the write protection through the Registry Editor.
2. Press Win + R, type **regedit**, and press Enter.
3. Navigate to: **HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\StorageDevicePolicies**
4. If you don't see **StorageDevicePolicies**, you may need to create it (see note below).
5. Double‑click the **WriteProtect DWORD**, set its Value data to **0**, and click OK.
6. Close the Registry Editor and restart your computer.      
*Note:* If the **StorageDevicePolicies** key doesn't exist, you can create it. Right‑click in the right pane, select **New > Key**, and name it exactly. Then inside it, create a new **DWORD (32‑bit) Value** named **WriteProtect**.

# 🛠️ Force format a corrupted SD card     

## 1. Diskpart Command Prompt (Advanced Format)
This method forcefully removes corrupted partitions and rebuilds the file system from scratch.
1. Insert the SD card into your PC.
2. Press the Windows Key + S, type **cmd**, right-click Command Prompt, and select Run as administrator.
3. Type **diskpart** and hit Enter.
4. Type **list disk** and press Enter. Carefully identify your SD card's disk number (e.g., Disk 1 or Disk 2) based on its size.
5. Type **select disk X** (replace X with your SD card's number) and hit Enter.
6. Type **attributes disk clear readonly** and press Enter.
7. Type **clean** and press Enter.
8. Type **create partition primary** and press Enter.
9. Type **format fs=exfat quick** (or **format fs=fat32 quick** for cards under 32GB) and press Enter.
10. Once completed, type **assign** to give the card a drive letter, then type **exit** to finish.

## 2. Disk Management Utility    
If diskpart fails, Windows Disk Management allows you to delete and recreate the volume.
1. Right-click the Start menu and select Disk Management.
2. Scroll to the bottom and locate your SD card.
3. Right-click the SD card's volume block and select Delete Volume (this removes the corrupted partition).
4. Right-click the newly unallocated black space and select New Simple Volume.
5. Follow the on-screen wizard to assign a drive letter and format it.

## 3. Dedicated Formatting Software   
If Windows tools fail, you can use specialized third-party formatting software designed to override errors.
- Use the official [SD Memory Card Formatter](https://www.sdcard.org/downloads/formatter/) by the SD Association, which is optimized exclusively for SD technology.
- Try low-level format software like [HDD Low Level Format Tool](http://hddguru.com/) to completely zero out the card's sector markers.


