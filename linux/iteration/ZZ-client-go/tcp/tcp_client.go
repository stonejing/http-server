package main

import (
	"fmt"
	"net"

	// "bufio"
	// "os"
	"strconv"
	"time"
)

func send(i int, c chan int) {
	conn, err := net.Dial("tcp", "127.0.0.1:8000")
	c <- i
	if err != nil {
		fmt.Println("err: ", err)
		return
	}

	defer conn.Close()
	text := "THIS IS A TEST FROM: " + strconv.Itoa(i)

	_, err = conn.Write([]byte(text))
	if err != nil {
		fmt.Println("write failed, err: ", err)
		return
	}

	buf := [512]byte{}
	_, err = conn.Read(buf[:])
	if err != nil {
		fmt.Println("recv failed, err: ", err)
		return
	}
	// fmt.Println("recv: ", i)
	// fmt.Println(string(buf[:n]))
	return
}

func main() {
	start := time.Now()
	fmt.Println("client: 127.0.0.1:8000")
	ch := make(chan int)
	num := 100
	for i := 0; i < num; i++ {
		go send(i, ch)
	}
	for i := 0; i < num; i++ {
		<-ch
	}
	// fmt.Println(<- ch)
	end := time.Now()
	fmt.Println("time: ", end.Sub(start))
	fmt.Println("END")
}
