FROM mcr.microsoft.com/vscode/devcontainers/universal:2-linux

# Install needed packages
RUN apt update -y && apt install -y libgtest-dev cmake

# Install GTest

RUN cd /usr/src/googletest && \
    cmake . && \
    cmake --build . --target install

RUN curl -fsSL https://starship.rs/install.sh | sh -s -- -y

USER 1000

RUN mkdir -p ~/.config/fish && \
    echo 'starship init fish | source' >> ~/.config/fish/config.fish && \
    echo 'set -g theme_nerd_fonts yes' >> ~/.config/fish/config.fish
