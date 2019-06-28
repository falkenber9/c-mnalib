ARG OS_VERSION=latest
FROM archlinux:$OS_VERSION as archlinux_base

# Provide command add-apt-repository and apt-utils
RUN pacman -Suy --noconfirm

# General dependencies for development
RUN pacman -Sy base-devel cmake --noconfirm

# Own dependencies
RUN pacman -Sy curl glib2 systemd-libs --noconfirm

ENTRYPOINT [ "stdbuf", "-o", "L" ]
