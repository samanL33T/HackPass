#pragma once

class IPlatformPaths;
class IPlatformSecurity;
class IPlatformUi;

class Platform {
public:
    static IPlatformPaths&    paths();
    static IPlatformSecurity& security();
    static IPlatformUi&       ui();

    Platform() = delete;
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;
};
