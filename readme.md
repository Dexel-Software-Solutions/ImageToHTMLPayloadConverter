# ImageToHTML Payload Converter

**A lightweight Windows desktop application for converting images into HTML-compatible payloads.**

ImageToHTML Payload Converter is a native Windows application developed in C. It provides a simple graphical interface for working with image data and converting it into HTML-compatible representations.

---

## ✨ Features

- 🖥️ **Native Windows GUI** — Built using the Windows API and C.
- 🖼️ **Image Processing Workflow** — Designed for working with image files.
- 🌐 **HTML-Compatible Output** — Converts image data into a format suitable for HTML-based workflows.
- ⚡ **Lightweight Application** — Native executable without requiring a separate runtime.
- 🎨 **Custom Application Icon** — Includes a custom Windows application icon.
- 📦 **Portable Executable** — Designed to run as a standalone Windows executable.
- 🔧 **Open Source** — Source code is available for inspection, modification, and development.

> **Note:** Actual functionality depends on the implementation in the current source code. Refer to the application and its source files for supported image formats and output options.

---

## 🖥️ Screenshots

Add screenshots of the application here.

```markdown
![Application Screenshot](screenshots/main-window.png)
```

---

## ⚙️ Requirements

### Running the application

- Microsoft Windows
- The compiled `ImageToHTMLPayloadConverter.exe` executable

### Building from source

- GCC compiler from MinGW-w64 / WinLibs
- GNU `windres` resource compiler
- Windows API development headers and libraries
- Command Prompt or PowerShell

---

## 🚀 Getting Started

### Option 1: Download the executable

1. Open the repository's **Releases** section.
2. Download the latest release package.
3. Extract the downloaded archive.
4. Run `ImageToHTMLPayloadConverter.exe`.

**Important:** A release package must contain the compiled executable before it can be used this way.

### Option 2: Build from source

Clone the repository:

```bash
git clone https://github.com/dexel-software-solutions/ImageToHTMLPayloadConverter.git
```

Navigate to the project directory:

```bash
cd ImageToHTMLPayloadConverter
```

Compile the Windows resource file:

```bash
windres app.rc -O coff -o app_res.o
```

Compile the application:

```bash
gcc -municode -O2 -Wall -Wextra -mwindows main.c app_res.o -o ImageToHTMLPayloadConverter.exe -lcomdlg32 -lcomctl32 -lshell32 -lgdi32
```

After a successful build, run:

```bash
ImageToHTMLPayloadConverter.exe
```

> **Build note:** Run these commands in a terminal where both `gcc` and `windres` are available in your system's `PATH`.

---

## 📁 Project Structure

```text
ImageToHTMLPayloadConverter/
├── main.c
├── app.rc
├── icon.ico
└──|_ ImageToHTMLPayloadConverter.exe
```

- `main.c` — Main application source code.
- `app.rc` — Windows application resources and version information.
- `icon.ico` — Application icon.
- `README.md` — Project documentation.
- `.gitignore` — Files excluded from version control.
- `ImageToHTMLPayloadConverter.exe` — Compiled Windows application, if distributed with the repository or release.

---

## 🛠️ Built With

- **C** — Core application implementation.
- **Windows API (Win32)** — Native graphical user interface.
- **GCC / MinGW-w64** — C compiler and linker.
- **GNU windres** — Windows resource compilation.

---

## 📦 Distribution

For a clean release, consider providing the following files through GitHub Releases:

```text
ImageToHTMLPayloadConverter-v1.0.0-Windows.zip
├── ImageToHTMLPayloadConverter.exe
└── README.txt
```

You may also publish the source code separately through the repository.

---

## 🗺️ Roadmap

Potential improvements for future versions:

- [ ] Add more image format support where applicable.
- [ ] Improve error handling and user feedback.
- [ ] Add output preview and copy-to-clipboard functionality.
- [ ] Improve accessibility and interface usability.
- [ ] Provide automated Windows release builds.
- [ ] Add application usage examples and screenshots.

---

## 🤝 Contributing

Contributions, bug reports, and suggestions are welcome.

1. Fork this repository.
2. Create a feature branch.
3. Make your changes.
4. Test your changes on Windows.
5. Submit a pull request describing your changes.

For substantial changes, consider opening an issue first to discuss the proposed implementation.

---

## 🐛 Bug Reports and Feature Requests

If you encounter a bug or have a feature request, please open an issue in the GitHub repository.

When reporting a bug, include:

- Windows version
- Application version
- Steps to reproduce the issue
- Expected behavior
- Actual behavior
- Relevant screenshots or error messages

---

## 👨‍💻 Developer

**Demiyan Dissanayake**

- **Organization:** Dexel Software Solutions
- **GitHub:** [@dexel-software-solutions](https://github.com/dexel-software-solutions)
- **Email:** demiyandissanayake@gmail.com

---

## 📄 License

No license has been specified in this repository yet.

Before distributing or accepting contributions to this project, add a `LICENSE` file containing the license you choose. For example, MIT is an option if you intend to allow reuse, modification, and redistribution under its terms.

---

## ⭐ Support the Project

If you find this project useful:

- Give the repository a ⭐ on GitHub.
- Report bugs and suggest improvements.
- Share the project with other developers.
- Contribute code or documentation.

---

**Developed by Demiyan Dissanayake | Dexel Software Solutions**

