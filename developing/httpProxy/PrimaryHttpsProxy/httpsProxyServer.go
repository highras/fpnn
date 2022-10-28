package main

import (
	"fmt"
	"os"
	"runtime"
	"./proxyServer"
)

func main() {

	if len(os.Args) != 4 {
		fmt.Println("Usage:", os.Args[0], "<listening-endpoint> <server-endpoint> <server-pem-key-file>")
		return
	}

	runtime.GOMAXPROCS(runtime.NumCPU())

	proxyServer.ConfigFpnnTransfer(os.Args[2], os.Args[3])
	proxyServer.Serve(os.Args[1])
}