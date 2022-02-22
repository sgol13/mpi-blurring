make && ./exe > out2
diff out1 out2
if [[ $? == 0 ]]; then
    echo "OK"
fi
