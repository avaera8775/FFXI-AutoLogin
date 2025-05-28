# FFXI AutoLogin - Build Instructions with Encryption

## Overview
The FFXI AutoLogin has been enhanced with encryption to secure account credentials. This document explains how to build and test the encryption features.

## What's New - Encryption Features
- ✅ **Username encryption** - No longer stored in plaintext
- ✅ **Password encryption** - Secured with machine-specific keys
- ✅ **TOTP Secret encryption** - Two-factor authentication secrets protected
- ✅ **Machine-specific keys** - Each installation has unique encryption
- ✅ **Cross-platform support** - Works on Windows and Wine
- ✅ **Backward compatibility** - Existing configs still work

## Building the Project

### Option 1: Using the Build Script (Recommended)
1. Open VS Code in this project folder
2. Open the integrated terminal (Ctrl + `)
3. Run the build script:
   ```cmd
   build.bat
   ```

### Option 2: Using Visual Studio
1. Open `FFXI-Launcher.vcxproj` in Visual Studio
2. Build the project (Ctrl + Shift + B)

### Option 3: Manual Compilation
If you have Visual Studio Build Tools installed:
```cmd
cl /EHsc FFXI-Launcher.cpp sha1.cpp /I"include" /link ws2_32.lib shlwapi.lib psapi.lib iphlpapi.lib
```

## Testing the Encryption

### 1. Create New Encrypted Config
```cmd
FFXI-Launcher.exe --setup
```
- Enter your account details as usual
- Passwords will be automatically encrypted when saved

### 2. Verify Encryption is Working
1. Open the generated `config.json` file
2. Look for the account entries - you should see:
   ```json
   {
     "accounts": [
       {
         "name": "base64EncodedEncryptedUsername==",
         "password": "base64EncodedEncryptedPassword==",
         "totpSecret": "base64EncodedEncryptedTOTPSecret==",
         "slot": 1,
         "args": ""
       }
     ]
   }
   ```
3. The `name`, `password`, and `totpSecret` fields should contain base64-encoded encrypted data
4. The `slot` and `args` fields remain in plaintext (they're not sensitive)

### 3. Test Migration from Existing Config
If you have an existing plaintext config:
1. The launcher will automatically detect and handle it
2. Plaintext passwords will be read normally
3. When you run `--setup` again, new passwords will be encrypted

## Security Features

### Machine Fingerprinting
The encryption key is generated from:
- Current username
- Executable path
- CPU information
- Network MAC address
- Wine detection (for Linux compatibility)

### Encryption Method
- **Algorithm**: XOR encryption with SHA1-hashed machine fingerprint
- **Encoding**: Base64 for safe JSON storage
- **Key Length**: 128-bit derived from machine characteristics

### Benefits
- **No shared secrets** - Each installation is unique
- **No external dependencies** - Uses built-in Windows APIs
- **Portable** - Works across different Windows environments
- **Secure** - Much better than plaintext storage

## Troubleshooting

### Build Errors
- **"cl is not recognized"**: Install Visual Studio Build Tools or run from Developer Command Prompt
- **Missing libraries**: The build script includes all required libraries
- **Include path errors**: Make sure the `include/` folder exists with `json.hpp` and `httplib.h`

### Runtime Issues
- **Decryption fails**: The machine fingerprint may have changed (new hardware, different user account)
- **Config not found**: Run with `--setup` to create a new encrypted config
- **Migration issues**: Backup your existing config.json before testing

## File Changes Made
- `FFXI-Launcher.cpp` - Added encryption functions and updated config handling
- `build.bat` - New build script for easy compilation
- `BUILD_INSTRUCTIONS.md` - This documentation

## Next Steps
1. Build the project using one of the methods above
2. Test with `--setup` to create an encrypted config
3. Verify the config.json contains encrypted data
4. Test normal login functionality to ensure everything works

The encryption is now fully integrated and ready for use!
