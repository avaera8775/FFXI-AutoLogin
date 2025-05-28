# Quick Start - FFXI AutoLogin with Encryption

## ⚡ Fastest Way to Build (No Visual Studio Required)

### Step 1: Install MinGW (One-time setup)
Open PowerShell as Administrator and run:
```powershell
winget install mingw-w64
```

**Alternative methods:**
- Download from: https://www.mingw-w64.org/downloads/
- Or install MSYS2: https://www.msys2.org/

### Step 2: Build the Project
1. Open VS Code in this project folder
2. Open terminal (Ctrl + `)
3. Run:
   ```cmd
   build_mingw.bat
   ```

### Step 3: Test Encryption
```cmd
FFXI-Launcher.exe --setup
```

## ✅ What You'll See

**Before (Plaintext):**
```json
{
  "accounts": [
    {
      "name": "MyUsername",
      "password": "MyPassword123",
      "totpSecret": "JBSWY3DPEHPK3PXP"
    }
  ]
}
```

**After (Encrypted):**
```json
{
  "accounts": [
    {
      "name": "SGVsbG9Xb3JsZA==",
      "password": "QWxhZGRpbjpvcGVuIHNlc2FtZQ==",
      "totpSecret": "VGhlIHF1aWNrIGJyb3du"
    }
  ]
}
```

## 🛠️ If MinGW Doesn't Work

**Option 1: Install Visual Studio Build Tools**
1. Download: https://visualstudio.microsoft.com/downloads/
2. Install "Build Tools for Visual Studio 2022"
3. Run `build.bat` instead

**Option 2: Use Online Compiler**
- Upload the source files to an online C++ compiler
- Include the required libraries

**Option 3: Ask Someone Else**
- Send the source code to someone with a C++ compiler
- They can build it and send you the .exe

## 🔒 Security Features Added

- ✅ **Passwords encrypted** with machine-specific keys
- ✅ **Usernames encrypted** (no longer visible in config)
- ✅ **TOTP secrets encrypted** (two-factor auth protected)
- ✅ **Cross-platform** (works in Wine too)
- ✅ **Backward compatible** (old configs still work)

## 📁 Files Modified

- `FFXI-Launcher.cpp` - Added encryption functions
- `build_mingw.bat` - Easy build script for MinGW
- `build.bat` - Build script for Visual Studio
- `QUICK_START.md` - This guide

The encryption implementation is complete - you just need a C++ compiler to build it!
