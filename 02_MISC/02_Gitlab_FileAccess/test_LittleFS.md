# Reusing `readMultilineText()` to Create Missing LittleFS Files

I've reviewed your current `.ino` code. You already have a reusable multiline Serial input function, `readMultilineText()`, which supports `.END` to finish input, `.CANCEL` to abort, and an 8 KiB text-input limit.

I recommend reusing that function rather than introducing a second Serial input mechanism.

We'll make two focused changes:

1. Add `prepareUploadTestFile()` — prompt for text over Serial and save it to a user-selected LittleFS path instead of writing hardcoded content.
2. Modify `uploadLittleFSFile()` — if the requested local file does not exist, ask whether to create it using the helper, then continue with the requested `UPLOAD` or `UPDATE` operation.

**One important distinction:** if the local file exists, the command should use it without prompting. If the remote file already exists during `UPLOAD`, GitLab should still reject the duplicate creation. Likewise, `UPDATE` should update an existing remote file, not create one accidentally.

## 1. Add a forward declaration

Your current `uploadLittleFSFile()` appears before the multiline input functions. Therefore, add a forward declaration before `uploadLittleFSFile()` so the compiler knows about the helper.

Insert this immediately before line 203, where `uploadLittleFSFile()` begins:

```cpp
// Forward declaration: implemented below readMultilineText().
static bool prepareUploadTestFile(const String& localPath);
```

This allows the upload/update function to call the helper even though its implementation appears later in the file.

## 2. Modify `uploadLittleFSFile()`

In your current implementation, the function immediately fails when `LittleFS.open()` cannot open the requested file.

Replace the existing `uploadLittleFSFile()` function with the following version. The main change is that a missing local file triggers the Serial input helper.

```cpp
static bool uploadLittleFSFile(const String& remotePath, const String& localPath, bool update) {
  if (!gitlab && !initializeGitlab()) return false;

  if (!littleFSReady) {  //if not mounted try again
    if (!LittleFS.begin(false)) {
      Serial.println("[LittleFS] Mount failed.");
      return false;
    }
    littleFSReady = true;
  }
  // Validate the local path.
  if (localPath.length() == 0 || !localPath.startsWith("/")) {
    Serial.println("Local source must be an absolute LittleFS path, e.g. /upload.txt");
    return false;
  }

  // Try to open the requested local file.
  File file = LittleFS.open(localPath, "r");
  if (!file || file.isDirectory()) {

    // BEGIN: Prompt user to create LittleFS file to flash
    if (file) file.close();

    Serial.printf(
      "[LittleFS] Local file '%s' does not exist or cannot be opened.\n",
      localPath.c_str());

    Serial.println("Create this local text file using Serial input? (Y/N)");

    String choice;
    if (!readSerialLine(choice, "> ")) {
      return false;
    }

    choice.trim();
    choice.toUpperCase();

    if (choice != "Y") {
      Serial.println("[CANCELLED] No file operation performed.");
      return false;
    }

    // Prompt for content and create the requested LittleFS file.
    if (!prepareUploadTestFile(localPath)) {
      Serial.println("[ERROR] Failed to prepare local file.");
      return false;
    }

    // Reopen the newly created file.
    file = LittleFS.open(localPath, "r");

    if (!file || file.isDirectory()) {
      if (file) file.close();

      Serial.println("[LittleFS] Cannot reopen the created file.");
      return false;
    }

    // END: Prompt user to create LittleFS file to flash
    /*	  
    Serial.println("[LittleFS] Cannot open local source file.");
    if (file) file.close();
    return false;
*/
  }

  // Check file size.
  size_t len = file.size();
  if (len == 0 || len > MAX_TEXT_UPLOAD_BYTES) {
    Serial.println("[Upload] Source file must be 1 byte to 256 KiB.");
    file.close();
    return false;
  }

  // Allocate PSRAM for the file contents.
  uint8_t* data = (uint8_t*)ps_malloc(len);
  if (!data) {
    Serial.println("[Upload] PSRAM allocation failed.");
    file.close();
    return false;
  }

  // Read the complete file.
  size_t got = file.read(data, len);
  file.close();
  if (got != len) {
    free(data);
    Serial.println("[Upload] Failed to read complete local file.");
    return false;
  }

  Serial.printf(
    "[%s] Local file: %s (%u bytes)\n",
    update ? "UPDATE" : "UPLOAD",
    localPath.c_str(),
    (unsigned)len);

  Serial.printf(
    "[%s] Remote file: %s\n",
    update ? "UPDATE" : "UPLOAD",
    remotePath.c_str());

  // Perform the requested GitLab operation.
  String commit = String(update ? "Update " : "Upload ") + remotePath + " from ESP32-S3";
  bool ok = update
              ? gitlab->updateFile(remotePath, data, len, commit)
              : gitlab->uploadNewFile(remotePath, data, len, commit);
  free(data);

  if (!ok) {
    Serial.printf(
      "[%s] Failed: %s\n",
      update ? "UPDATE" : "UPLOAD",
      gitlab->lastError().c_str());
    return false;
  }

  Serial.printf(
    "[%s] Successful.\n",
    update ? "UPDATE" : "UPLOAD");

  return true;

  /*
  if (!ok) Serial.println(gitlab->lastError());
  return ok;
  */
}
```

**Note:** This version uses `MAX_TEXT_UPLOAD_BYTES`, which your existing code defines as 256 KiB. It also uses `readSerialLine()`, which is already implemented in your sketch.

## 3. Add `prepareUploadTestFile()`

Place the following implementation after your existing `readMultilineText()` function, around line 365, and before `runTextUpload()`.

This reuses your existing multiline input function rather than duplicating its logic.

```cpp
/ Create a text file in LittleFS using content entered over Serial.
static bool prepareUploadTestFile(const String& localPath) {

  Serial.println();
  Serial.println("========== CREATE LOCAL TEST FILE ==========");
  Serial.printf("LittleFS path: %s\n", localPath.c_str());

  // NEW: Create the parent directory if it does not exist.
  int lastSlash = localPath.lastIndexOf('/');
  if (lastSlash > 0) {
    String parentDir = localPath.substring(0, lastSlash);

    if (!LittleFS.exists(parentDir)) {
      Serial.printf(
        "[LittleFS] Creating parent directory: %s\n",
        parentDir.c_str());

      if (!LittleFS.mkdir(parentDir)) {
        Serial.printf(
          "[LittleFS] Failed to create parent directory: %s\n",
          parentDir.c_str());
        return false;
      }

      Serial.println("[LittleFS] Parent directory created.");
    }
  }
  // END NEW

  Serial.println("Enter the file content below.");
  Serial.println("Enter .END on a line by itself to finish.");
  Serial.println("Enter .CANCEL on a line by itself to abort.");

  String content;

  // Reuse the existing multiline Serial input function.
  if (!readMultilineText(content)) {
    Serial.println("[CANCELLED] File creation aborted.");
    return false;
  }

  // Enforce the upload size limit.
  if (content.length() > MAX_TEXT_UPLOAD_BYTES) {
    Serial.println("[ERROR] Content exceeds 256 KiB.");
    return false;
  }

  // Create or overwrite the local file.
  File file = LittleFS.open(localPath, "w");

  if (!file || file.isDirectory()) {
    if (file) file.close();

    Serial.println("[LittleFS] Failed to create local file.");
    return false;
  }

  const size_t expected = content.length();

  const size_t written = file.write(
    reinterpret_cast<const uint8_t*>(content.c_str()),
    expected);

  file.close();

  if (written != expected) {
    Serial.printf(
      "[LittleFS] Incomplete write: %u of %u bytes.\n",
      (unsigned)written,
      (unsigned)expected);

    return false;
  }

  Serial.printf(
    "[LittleFS] Created '%s' (%u bytes).\n",
    localPath.c_str(),
    (unsigned)written);

  Serial.println("[LittleFS] File content saved successfully.");

  return true;
}
```

### How the helper works

For example, suppose you enter:

```text
UPLOAD test/upload_test.txt /upload_test.txt
```

If `/upload_test.txt` does not exist, the ESP32 prompts:

```text
[LittleFS] Local file '/upload_test.txt' does not exist or cannot be opened.
Create this local text file using Serial input? (Y/N)
>
```

Enter:

```text
Y
```

Then enter your content:

```text
ESP32-S3 GitLab upload test
Version: 1
Testing upload from LittleFS
.END
```

The ESP32 writes these lines to `/upload_test.txt` in LittleFS, then continues with the original `UPLOAD` command.

The `.END` line is a terminator and is not saved in the file. Each preceding input line is stored with a trailing newline, as implemented by your existing `readMultilineText()` function.

If you enter `.CANCEL`, the helper aborts without creating the file.

## 4. How to test the changes

After compiling and uploading the modified sketch, use the following sequence in Serial Monitor.

Set the baud rate to `115200` and line ending to `Newline`.

### Test A — `UPLOAD` a new file

Enter:

```text
UPLOAD test/upload_test.txt /upload_test.txt
```

If the local file doesn't exist, enter `Y` when prompted, then provide:

```text
ESP32-S3 GitLab upload test
Version: 1
.END
```

Expected sequence:

1. The ESP32 detects that the local file is missing.
2. It prompts you to create the file.
3. You enter the text.
4. LittleFS saves the text.
5. The ESP32 reads the file from LittleFS.
6. The ESP32 attempts to create the remote GitLab file.

Verify that `test/upload_test.txt` appears in your GitLab repository with the expected contents.

### Test B — `UPDATE` the same file

Now modify the local file by deleting `/upload_test.txt` using an appropriate local-file management command, or use a different local path.

For example:

```text
UPDATE test/upload_test.txt /update_test.txt
```

When prompted, enter:

```text
ESP32-S3 GitLab upload test
Version: 2
Testing UPDATE from LittleFS
.END
```

The helper creates `/update_test.txt`, and the ESP32 uses its contents to update the existing remote file `test/upload_test.txt`.

Verify that the remote file now contains `Version: 2`.

### Test C — Verify existing local files don't prompt

Run:

```text
UPLOAD test/another_file.txt /update_test.txt
```

Since `/update_test.txt` now exists, the ESP32 should skip the file-creation prompt and proceed directly to the GitLab operation.

If `test/another_file.txt` doesn't exist remotely, GitLab should create it. If it already exists, `UPLOAD` should fail rather than silently overwrite it.

## 5. Two design considerations

**First:** the helper is intended for text files. Your existing `readMultilineText()` reads line-oriented text, so it is suitable for `.txt`, JSON, CSV, and similar files. It is not suitable for arbitrary binary files such as `.png`, `.bin`, or firmware images. Those need a different Serial transfer mechanism.

**Second:** a failed `UPDATE` must not silently become an `UPLOAD`. The modified function preserves the distinction by calling `gitlab->updateFile()` for `UPDATE` and `gitlab->uploadNewFile()` for `UPLOAD`. However, the final remote behavior also depends on how those methods are implemented in `GitlabClient.cpp`.

### One further improvement I recommend

Your existing `readMultilineText()` limits interactive input to 8 KiB, while the upload function allows files up to 256 KiB.

That is fine for testing, but the practical limit for files created using this helper is 8 KiB, not 256 KiB. Keep the two limits distinct: the larger limit allows existing LittleFS files to be uploaded, while the smaller limit bounds interactive text entry.

**Result:** You can now create a missing LittleFS text file directly from Serial Monitor and immediately use it as the source for either `UPLOAD` or `UPDATE`, without hardcoding its contents into the sketch.

---
# Adding a `LISTLOCAL` Command for LittleFS

I recommend adding a `LISTLOCAL` command that recursively lists all files and subdirectories in LittleFS, including each file's size. It will not require Wi-Fi or GitLab access.

## 1. Add the helper function

Add this function to your `.ino` file, in the helper-functions section. It uses the existing LittleFS library.

```cpp
// Recursively list files and subdirectories in LittleFS.
static void listLocalDirectory(const String& path, uint8_t depth) {
  File dir = LittleFS.open(path, "r");

  if (!dir || !dir.isDirectory()) {
    Serial.printf("[ERROR] Cannot open directory: %s\n", path.c_str());
    if (dir) dir.close();
    return;
  }

  File entry = dir.openNextFile();

  while (entry) {
    String name = entry.name();
    bool isDir = entry.isDirectory();
    size_t fileSize = entry.size();

    // Print indentation.
    for (uint8_t i = 0; i < depth; i++) {
      Serial.print("  ");
    }

    // Normalize entry name into a full path.
    String childPath;

    if (name.startsWith("/")) {
      childPath = name;
    } else {
      childPath = path;
      if (!childPath.endsWith("/")) {
        childPath += "/";
      }
      childPath += name;
    }

    if (isDir) {
      Serial.printf("[DIR]  %s/\n", childPath.c_str());

      // Close the current entry before recursively opening its directory.
      entry.close();

      listLocalDirectory(childPath, depth + 1);
    } else {
      Serial.printf("[FILE] %s (%u bytes)\n",
                    childPath.c_str(),
                    (unsigned)fileSize);

      entry.close();
    }

    entry = dir.openNextFile();
  }

  dir.close();
}

// LISTLOCAL: list all local files, including subdirectories.
static void runListLocal() {
  Serial.println();
  Serial.println("========== LOCAL LITTLEFS FILES ==========");

  if (!littleFSReady) {
    Serial.println("[ERROR] LittleFS is not mounted.");
    return;
  }

  Serial.printf("Filesystem total: %u bytes\n",
                (unsigned)LittleFS.totalBytes());
  Serial.printf("Filesystem used:  %u bytes\n",
                (unsigned)LittleFS.usedBytes());

  listLocalDirectory("/", 0);

  Serial.println("========== END OF LOCAL FILES ==========");
}
```

**Note:** This implementation assumes you have a global `littleFSReady` flag that is set to `true` after LittleFS mounts successfully. Your `setup()` code should maintain that flag.

## 2. Register the command in `handleCommand()`

Find your existing command handling code:

```cpp
if (command == "HELP") {
  printHelp();
} else if (command == "DEL" && argument.equalsIgnoreCase("token")) {
```

Insert the new command between `HELP` and `DEL`:

```cpp
if (command == "HELP") {
  printHelp();
} else if (command == "LISTLOCAL") {
  runListLocal();
} else if (command == "DEL" && argument.equalsIgnoreCase("token")) {
```

## 3. Update `printHelp()`

Add this line to the command list:

```cpp
Serial.println(" LISTLOCAL           ->list all local LittleFS files recursively");
```

## 4. Example output

Suppose LittleFS contains the following files:

```text
/
├── upload_test.txt
├── config.txt
└── assets/
    ├── logo.png
    └── documents/
        └── readme.txt
```

Entering this command in the Serial Monitor:

```text
LISTLOCAL
```

Could produce output similar to:

```text
========== LOCAL LITTLEFS FILES ==========
Filesystem total: 10354688 bytes
Filesystem used:  16384 bytes
[FILE] /upload_test.txt (128 bytes)
[FILE] /config.txt (64 bytes)
[DIR]  /assets/
  [FILE] /assets/logo.png (2048 bytes)
  [DIR]  /assets/documents/
    [FILE] /assets/documents/readme.txt (512 bytes)
========== END OF LOCAL FILES ==========
```

The actual capacity, used space, and file sizes depend on your filesystem contents.

### Important implementation detail

Arduino-ESP32 filesystem implementations can differ in whether `entry.name()` returns a basename or a full path. If it returns a full path, concatenating it with the parent directory would construct an incorrect child path.

If your output shows incorrect paths or directories aren't traversed, print `entry.name()` for a nested file and adjust the path construction accordingly.

**Expected benefit:** `LISTLOCAL` lets you verify that a file exists in LittleFS before running commands such as `UPLOAD` or `UPDATE`.
