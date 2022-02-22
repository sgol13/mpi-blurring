# make && valgrind ./exe > out2
make && mpirun -np 4 ./exe > out2
diff out1 out2
if [[ $? == 0 ]]; then
    echo "OK"
fi
