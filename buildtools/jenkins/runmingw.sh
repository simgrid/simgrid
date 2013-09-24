#!c:\mingw\msys\1.0\bin\sh -login

WORKSPACE=$1
build_mode=$2

rm -rf $WORKSPACE/build
rm -rf $WORKSPACE/install
mkdir $WORKSPACE/build
mkdir $WORKSPACE/install
cd $WORKSPACE/build

if [ "$build_mode" = "Debug" ]
then
cmake -G "MSYS Makefiles" ..
fi

if [ "$build_mode" = "ModelChecker" ]
then
cmake -G "MSYS Makefiles" -Denable_model-checking=ON -Denable_compile_optimizations=OFF ..
fi

make
