# FFXI AutoLogin - Modern GUI Version

A sleek, modern Windows GUI for the FFXI AutoLogin tool featuring a dark theme and professional interface.

## 🎨 Modern Design Features

- **Dark Theme**: Professional dark color scheme with blue accents
- **Rounded Corners**: Modern flat design with subtle shadows
- **Custom Controls**: Owner-drawn buttons and interface elements
- **Smooth Animations**: Hover effects and visual feedback
- **Gaming Aesthetic**: Designed to fit with modern gaming applications

## 🖼️ Interface Preview

**Main Window:**
- Character management with visual cards
- One-click launch buttons for each character
- Clean, organized layout
- Settings and about dialogs

**Setup Dialog:**
- Form-based character configuration
- Password masking for security
- Dropdown slot selection
- Real-time validation

## 🔧 Building the GUI

### Requirements
- Visual Studio 2022 (Community or Build Tools)
- Windows SDK
- GDI+ (included with Windows)

### Build Steps
1. Open Developer Command Prompt
2. Navigate to the `gui/` folder
3. Run the build script:
   ```cmd
   build_gui.bat
   ```

### Manual Build
```cmd
cl /EHsc gui_main.cpp theme.cpp ../sha1.cpp /I"../include" /link user32.lib gdi32.lib comctl32.lib gdiplus.lib ws2_32.lib shlwapi.lib psapi.lib iphlpapi.lib /OUT:FFXI-Launcher-GUI.exe
```

## 🚀 Usage

1. **Launch the Application**
   ```cmd
   FFXI-Launcher-GUI.exe
   ```

2. **Add Characters**
   - Click "Setup Characters" button
   - Fill in character details:
     - Character Name (unique, no spaces)
     - Password (automatically encrypted)
     - TOTP Secret (optional, for 2FA)
     - Character Slot (1-4)
     - Windower Arguments (optional)
   - Click "Add Character"

3. **Launch Characters**
   - Select character from the main window
   - Click the "Launch" button on their card
   - The application will automatically handle login

## 🔒 Security Features

- **Encrypted Storage**: All passwords and TOTP secrets are encrypted
- **Machine-Specific Keys**: Encryption tied to your specific computer
- **Visual Password Masking**: Passwords hidden during input
- **Secure Memory Handling**: Sensitive data cleared from memory

## 🌍 Cross-Platform Compatibility

- **Windows Native**: Full functionality on Windows 10/11
- **Wine Compatible**: Works in Wine on Linux
- **Modern Look**: Consistent appearance across platforms

## 🎮 Gaming Integration

- **Windower Support**: Full integration with Windower arguments
- **Multiple Characters**: Manage unlimited characters
- **Quick Launch**: One-click character launching
- **TOTP Support**: Built-in two-factor authentication

## 📁 File Structure

```
gui/
├── gui_main.cpp          # Main application and window handling
├── theme.h               # Modern theme definitions
├── theme.cpp             # Theme rendering functions
├── build_gui.bat         # Build script
└── README_GUI.md         # This file
```

## 🔧 Technical Details

**Framework**: Win32 API with GDI+ for modern graphics
**Theme**: Custom dark theme with blue accents
**Controls**: Owner-drawn for consistent modern appearance
**Encryption**: Same secure system as console version
**Memory**: Efficient resource management

## 🆚 GUI vs Console Version

| Feature | GUI Version | Console Version |
|---------|-------------|-----------------|
| Interface | Modern visual | Text-based |
| Setup | Form-based | Command prompts |
| Character Management | Visual cards | Text list |
| User Experience | Point & click | Keyboard input |
| Appearance | Modern dark theme | Terminal window |
| Functionality | Identical | Identical |

## 🐛 Troubleshooting

**Build Issues:**
- Ensure you're in a Developer Command Prompt
- Check that Visual Studio Build Tools are installed
- Verify include paths are correct

**Runtime Issues:**
- Run as administrator if needed
- Check that GDI+ is available (should be on all modern Windows)
- Ensure config.json is writable

## 🎯 Future Enhancements

- [ ] Character import/export
- [ ] Themes and customization
- [ ] Launch scheduling
- [ ] Multi-monitor support
- [ ] Tray icon integration

The GUI version provides the same powerful encryption and automation features as the console version, but with a modern, user-friendly interface that fits perfectly with today's gaming applications.
