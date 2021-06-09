
Debian
====================
This directory contains files used to package cerebellumd/cerebellum-qt
for Debian-based Linux systems. If you compile cerebellumd/cerebellum-qt yourself, there are some useful files here.

## cerebellum: URI support ##


cerebellum-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install cerebellum-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your cerebellum-qt binary to `/usr/bin`
and the `../../share/pixmaps/cerebellum128.png` to `/usr/share/pixmaps`

cerebellum-qt.protocol (KDE)

