set -x

make clean
qmake davincitest.pro
make
./davincitest
make clean
rm davincitest
