g++ server_1104_2.cpp -o server

if [ "$?" == "0" ]; then
	./server
else
	echo "fail"
fi
