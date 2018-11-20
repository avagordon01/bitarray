if [ ! -d out ]; then
    CXX=clang++ \
    meson out \
    -Db_sanitize=address -Db_lundef=false \
    --buildtype debugoptimized
fi
ninja -C out &&
out/bitarray-test
