
<div align="center">

![C++](https://img.shields.io/badge/Language-C%2B%2B-blue?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Windows%20Win32-0078D6?style=for-the-badge&logo=windows&logoColor=white)
![UI](https://img.shields.io/badge/UI-GDI%2B%20%2F%20DWM-indigo?style=for-the-badge)
![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?style=for-the-badge&logo=discord&logoColor=white)

**Utilitas otomasi Windows native (C++/Win32) yang dirancang untuk menjaga stabilitas, performa, dan status aktif multi-instance permainan Roblox secara efisien.**

[Unduh rilis terbaru](https://github.com/habNoir/NotAFK-HAB) • [Laporkan Bug](https://github.com/habNoir/NotAFK-HAB) • [Donasi](https://discord.gg/jcfFAvKWvF)

</div>

---

## 📖 Tentang Proyek

**NotAFK-HAB** adalah aplikasi utilitas berbasis C++ murni yang beroperasi langsung di atas Windows API untuk mengelola aktivitas permainan Roblox. Tidak seperti makro keyboard biasa, aplikasi ini dirancang secara modular untuk memanipulasi jendela permainan, membatasi penggunaan daya CPU/GPU saat tidak fokus, mengontrol audio di tingkat subsistem Windows, serta mendeteksi pemutusan jaringan secara pintar untuk melakukan koneksi ulang (*auto-reconnect*).

Proyek ini menggunakan arsitektur modular yang memisahkan logika utama (*engine loop*), pengontrol grafis (GDI+), manajemen audio, pemantauan input, dan sistem notifikasi eksternal (Discord Webhooks).

---

## 🛠️ Fitur & Implementasi Teknis

Aplikasi ini dibangun menggunakan modul-modul independen yang saling berkomunikasi secara efisien:

### 1. Detektor & Multi-Instance (`RobloxDetector`)
* **Bypass Mutex:** Memanipulasi Mutex global Windows (`ROBLOX_singletonEvent`) untuk melewati batas satu instans aplikasi Roblox pada satu komputer.
* **Smart Grid Snapping:** Mendeteksi semua jendela Roblox aktif, menghitung rasio aspek layar monitor, dan secara dinamis mengubah ukuran serta memosisikan semua jendela tersebut ke dalam grid desktop yang rapi.
* **Dukungan Spoofing:** Mendukung deteksi proses alternatif untuk utilitas optimalisasi seperti Fishstrap (`eurotrucks2.exe`).

### 2. Mesin Anti-Idle Utama (`AntiAFKEngine` & `InputSimulator`)
* **Simulasi Input Aman:** Menghindari deteksi idle Roblox dengan mengirimkan input keyboard (`Space`, `W/S`, `Zoom In/Out`) langsung ke antrean pesan input sistem operasi menggunakan fungsi `SendInput`.
* **Deteksi Layar Putus (Auto-Reconnect):** Mendeteksi layar pemutusan koneksi (*disconnected screen*) dengan memeriksa kesesuaian nilai warna piksel (`RGB(57, 59, 61)`) pada koordinat tengah jendela Roblox, lalu melakukan simulasi pergerakan kursor halus (*smooth mouse interpolation*) untuk menekan tombol Reconnect.
* **Pemulihan Fokus Jendela:** Setelah mengirimkan input ke Roblox, sistem secara otomatis mengembalikan fokus ke jendela aplikasi yang sedang Anda gunakan sebelumnya menggunakan metode Alt-Tab atau penataan ulang posisi z-order.

### 3. Pengendali Frame Rate (`FpsCapper`)
* **Thread Suspension:** Mengurangi konsumsi daya komputer secara drastis saat Anda sedang AFK dengan menangguhkan thread proses Roblox (`SuspendThread`) untuk membatasi frame rate game hingga 3/5/15 FPS di latar belakang.
* **Auto-Unlock:** FPS akan otomatis dibuka kunci (*unlocked*) kembali saat Anda memfokuskan jendela game tersebut.

### 4. Manajemen Audio (`AudioManager`)
* **Mute Otomatis:** Memanfaatkan *Windows Core Audio APIs* (`IAudioSessionManager2` & `ISimpleAudioVolume`) untuk membisukan audio proses Roblox yang berjalan di latar belakang tanpa mengganggu jalannya permainan.

### 5. Notifikasi Discord (`DiscordWebhook`)
* **Rich Embeds:** Mengirim log status aktivitas (mesin aktif, durasi sesi berjalan, tindakan anti-idle yang dikirim, atau kesalahan ketika game tertutup) berupa format embed berwarna ke saluran Discord Anda menggunakan koneksi HTTPS asinkron (`WinINet`).

---

## 📂 Struktur Kode Proyek

```text
├── core/
│   ├── AntiAFKEngine.cpp / .h     <- Alur kerja dan siklus logika utama pencegah AFK
│   ├── InputSimulator.cpp / .h    <- Abstraksi pengiriman input tingkat rendah (Win32 SendInput)
│   ├── RobloxDetector.cpp / .h    <- Manipulasi Mutex Roblox, deteksi window, dan Grid Snapping
│   ├── FpsCapper.cpp / .h         <- Pengatur batas FPS melalui metode Thread Suspension
│   └── AudioManager.cpp / .h      <- Interaksi Core Audio API untuk auto-mute/unmute proses
├── ui/
│   ├── MainWindow.cpp / .h        <- Desain antarmuka GDI+ bertema Obsidian Gelap (Dark Mode)
│   ├── StatusBarOverlay.cpp / .h  <- Notifikasi overlay transparan & tembus klik (click-through)
│   ├── UIStyle.h                  <- Palet warna program dan struktur gaya UI
│   └── AnimationHelper.h          <- Kelas utilitas interpolasi (Lerp) untuk animasi visual
├── network/
│   └── DiscordWebhook.cpp / .h    <- Pengirim log status berbasis HTTP POST asinkron (WinINet)
├── utils/
│   └── ConfigManager.cpp / .h     <- Pengelola berkas konfigurasi preset (JSON parser & exporter)
├── resource.h / .rc               <- Pemetaan resource ikon baki sistem dan ikon aplikasi
└── links.json                     <- Berkas rujukan tautan resmi eksternal aplikasi

🎨 Tampilan UI & Gaya Visual

Antarmuka NotAFK-HAB didesain menggunakan skema warna Obsidian Gelap / Digital
Indigo Glow:

  - Latar Belakang Utama (COLOR_BG_DARK): Deep Obsidian (RGB(12, 12, 16))
  - Panel Kartu (COLOR_CARD_DARK): Carbon Gray (RGB(22, 22, 30))
  - Glow Aksen Utama (COLOR_ACCENT_PRIMARY): Digital Indigo (RGB(99, 102, 241))
  - Glow Aksen Sekunder (COLOR_ACCENT_SECONDARY): Cyber Teal (RGB(6, 182, 212))

Aplikasi ini menggunakan perataan sudut membulat modern yang terintegrasi dengan
gaya visual Windows 11 melalui atribut DWM API.

🛠️ Langkah Kompilasi (Build)

Prasyarat:

1.  Microsoft Visual Studio 2022 (dengan paket beban kerja Desktop development
    with C++).
2.  Windows 10/11 SDK.

Prosedur:

1.  Clone repositori ini ke penyimpanan lokal Anda.
2.  Buka folder proyek di Visual Studio.
3.  Pastikan konfigurasi Build diatur pada mode Release dengan arsitektur x64
    (atau x86 jika diperlukan).
4.  Build proyek dengan menekan tombol Ctrl + Shift + B.
5.  Hasil kompilasi berupa file .exe mandiri dapat ditemukan di folder output
    rilis Anda.

⚙️ Berkas Konfigurasi Preset (preset.json)

Konfigurasi aplikasi dapat diekspor atau diimpor secara langsung dalam bentuk
file JSON. Berikut adalah struktur dasar konfigurasinya:

{
  "language": 1,
  "multiSupport": true,
  "fishstrapSupport": false,
  "selectedTime": 540,
  "selectedAction": 0,
  "userSafeMode": 0,
  "autoStartAfk": false,
  "autoReconnect": true,
  "autoReset": false,
  "autoHideRoblox": false,
  "restoreMethod": 1,
  "fpsLimit": 15,
  "unlockFpsOnFocus": true,
  "multiInstanceInterval": 0,
  "autoMute": true,
  "discordWebhookEnabled": true,
  "discordWebhookUrl": "ISI_URL_WEBHOOK_DISCORD_ANDA"
}

🔗 Tautan Resmi

Berikut adalah tautan rujukan resmi untuk proyek NotAFK-HAB:

  - 💬 Discord Server: Gabung Komunitas
  - 💻 GitHub Repo: habNoir/NotAFK-HAB
  - 📖 Panduan & Wiki: Akses Wiki Proyek
  - 📦 Sourceforge: Alternatif Unduhan
  - ☕ Dukung Kami: Beri Donasi / Tips

📄 Lisensi & Disclaimer

  - Lisensi: Proyek ini dilisensikan di bawah Lisensi MIT.
  - Disclaimer: Utilitas ini dikembangkan untuk tujuan efisiensi manajemen daya
    dan stabilitas permainan. Harap gunakan fitur pencegah AFK secara bijaksana.
    Pengembang tidak bertanggung jawab atas tindakan penertiban atau konsekuensi
    akun yang mungkin timbul dari penyalahgunaan aplikasi ini di dalam
    permainan
