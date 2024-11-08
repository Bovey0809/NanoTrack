dirname="build"
if [ ! -d "$dirname" ]; then
    mkdir "$dirname"
else
    echo "dir exist"
fi
cd "$dirname"

cmake -DCMAKE_SYSTEM_PROCESSOR=arm64 \
      -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib" \
      -DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib" \
      -DOpenMP_C_LIB_NAMES="omp" \
      -DOpenMP_CXX_LIB_NAMES="omp" \
      -DOpenMP_omp_LIBRARY="/opt/homebrew/opt/libomp/lib/libomp.dylib" \
      ..

make -j8

cd ../

# DCMAKE_BUILD_TYPE=Release 