package main

import (
	"net"
	"fmt"
	// "bufio"
	// "os"
	"strconv"
)

func send(i int, c chan int) {
	conn, err := net.Dial("tcp", "127.0.0.1:8000")
	if err != nil {
		fmt.Println("err: ", err)
		c <- i
		return
	}

	defer conn.Close()
	text := "THIS IS A TEST FROM: " + strconv.Itoa(i)

	_, err = conn.Write([]byte(text))
	if err != nil {
		return
	}

	buf := [512]byte{}
	n, err := conn.Read(buf[:])
	if err != nil {
		fmt.Println("recv failed, err: ", err)
		return
	}
	fmt.Println(string(buf[:n]))
	return
}

func main() {
	fmt.Println("client: 127.0.0.1:8000")
	ch := make(chan int)
	for i := 0; i < 1000; i++ {
		go send(i, ch)
	}
	fmt.Println(<- ch)
	fmt.Println("END")
}