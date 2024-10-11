# Clone vcpkg
git clone https://github.com/microsoft/vcpkg.git

# Build only Release libraries, skip Debug
echo 'set(VCPKG_BUILD_TYPE release)' >>vcpkg/triplets/x86-windows.cmake

# Build with Win7 support enabled
echo 'set(VCPKG_CXX_FLAGS "-D_WIN32_WINNT=0x0601 -DWINVER=0x0601")' >>vcpkg/triplets/x86-windows.cmake
echo 'set(VCPKG_C_FLAGS   "-D_WIN32_WINNT=0x0601 -DWINVER=0x0601")' >>vcpkg/triplets/x86-windows.cmake

# Initialize
vcpkg/bootstrap-vcpkg.bat

# Perform actual build
vcpkg/vcpkg.exe install tbb fuzzylite sdl2 sdl2-image sdl2-ttf sdl2-mixer[core,mpg123] qt5-base ffmpeg[core,avcodec,avformat,swresample,swscale] qt5-tools boost-filesystem boost-system boost-thread boost-program-options boost-locale boost-iostreams boost-headers boost-foreach boost-format boost-crc boost-logic boost-multi-array boost-ptr-container boost-heap boost-bimap boost-asio boost-stacktrace boost-assign boost-geometry boost-uuid boost-uuid boost-process --triplet x86-windows

# Create exported archive
vcpkg/vcpkg.exe export tbb fuzzylite sdl2 sdl2-image sdl2-ttf sdl2-mixer qt5-base ffmpeg qt5-tools boost-filesystem boost-system boost-thread boost-program-options boost-locale boost-iostreams boost-headers boost-foreach boost-format boost-crc boost-logic boost-multi-array boost-ptr-container boost-heap boost-bimap boost-asio boost-stacktrace boost-assign boost-geometry boost-uuid boost-process --raw --triplet x86-windows --output=out/vcpkg

tar --create --xz --file dependencies-msvc-x86.txz -C vcpkg/out vcpkg

# Log list of packages
vcpkg/vcpkg.exe list

DUMPBIN_DIR=$(vswhere -latest -find **/dumpbin.exe | head -n 1)
dirname "$DUMPBIN_DIR" > $GITHUB_PATH
