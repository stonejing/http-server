package main

import (
	"net"
	"fmt"
	// "bufio"
	// "os"
	"strconv"
	"time"
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
	c <- -1
	return
}

func main() {
	start := time.Now()
	fmt.Println("client: 127.0.0.1:8000")
	ch := make(chan int)
	for i := 0; i < 50; i++ {
		go send(i, ch)
	}
	for i := 0; i < 50; i++ {
		<-ch
	}
	// fmt.Println(<- ch)
	end := time.Now()
	fmt.Println("time: ", end.Sub(start))
	fmt.Println("END")
}