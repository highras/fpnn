package proxyServer

import (
	"io"
	"net"
	"net/http"
	"sync"
	"time"
)

type RouteDictionary struct {
	mutex sync.Mutex
	routeMap map[string]bool
}

func newRouteDictionary() *RouteDictionary {
	dict := new(RouteDictionary)
	dict.routeMap = make(map[string]bool)
	return dict
}

func (dict *RouteDictionary)insertRoute(host string, route bool) {
	dict.mutex.Lock()
	dict.routeMap[host] = route
	dict.mutex.Unlock()
}

func (dict *RouteDictionary)getRoute(host string) (bool, bool) {
	dict.mutex.Lock()
	val, ok := dict.routeMap[host]
	dict.mutex.Unlock()

	return val, ok
}

var GlobalRouteDictionary = newRouteDictionary()

func dialTest(host string) (net.Conn, bool) {
	destConn, err := net.DialTimeout("tcp", host, 10*time.Second)
	if err != nil {
		GlobalRouteDictionary.insertRoute(host, true)
		return nil, false
	} else {
		GlobalRouteDictionary.insertRoute(host, false)
		return destConn, true
	}
}

func Proxy(w http.ResponseWriter, r *http.Request){

	var conn net.Conn = nil
	route, ok := GlobalRouteDictionary.getRoute(r.Host)
	if !ok {
		conn, route = dialTest(r.Host)
	}

	if r.Method == http.MethodConnect {
		
		if route {
			if conn != nil {
				conn.Close()
			}
			fpnnHandleHttps(w, r)
		} else {
			handleHttps(conn, w, r)
		}

	} else {

		if conn != nil {
			conn.Close()
		}

		if route {
			fpnnHandleHttp(w, r)
		} else {
			handleHttp(w, r)
		}
	}
}

func handleHttps(destConn net.Conn, w http.ResponseWriter, r *http.Request){
	if destConn == nil {
		conn, err := net.DialTimeout("tcp", r.Host, 60*time.Second)
		if err != nil {
			http.Error(w, err.Error(), http.StatusServiceUnavailable)
			return
		}
		destConn = conn
	}
		
	w.WriteHeader(http.StatusOK)

	hijacker, ok := w.(http.Hijacker)
	if !ok {
		http.Error(w, "Hijacking not supported", http.StatusInternalServerError)
		return
	}

	clientConn, _, err := hijacker.Hijack()
	if err != nil {
		http.Error(w, err.Error(), http.StatusServiceUnavailable)
	}
	go transfer(destConn, clientConn)
	go transfer(clientConn, destConn)

}

func handleHttp(w http.ResponseWriter, r *http.Request){
	resp, err := http.DefaultTransport.RoundTrip(r)
	if err != nil {
		http.Error(w, err.Error(), http.StatusServiceUnavailable)
		return
	}
	defer resp.Body.Close()

	copyHeader(w.Header(), resp.Header)
	w.WriteHeader(resp.StatusCode)
	io.Copy(w, resp.Body)

}

func transfer(destination io.WriteCloser, source io.ReadCloser) {
	defer destination.Close()
	defer source.Close()
	io.Copy(destination, source)
}

func copyHeader(dst, src http.Header) {
	for k, vv := range src {
		for _, v := range vv {
			dst.Add(k, v)
		}
	}
}
