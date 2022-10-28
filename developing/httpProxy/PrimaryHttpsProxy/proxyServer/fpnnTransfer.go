package proxyServer

import (
	"fmt"
	"io"
	"io/ioutil"
	"net"
	"net/http"
	"strconv"
	"strings"
	"sync"
	"time"
	"github.com/highras/fpnn-sdk-go/src/fpnn"
)

//=====================================================================//
//--                         Global Structure                        --//
//=====================================================================//
type ProxyClient struct {
	mutex sync.Mutex
	client *fpnn.TCPClient
	lastActiveTs int64
}

type SessionInfo struct {
	mutex		sync.Mutex
	conn		net.Conn
	sessionId	uint32
	sendSeq		uint32
	recvSeq		uint32
	recvMap		map[uint32][]byte
}

type ProxyQuestProcessor struct {}

var (
	secondaryProxyServerEndpoint = ""
	secondaryProxyServerPemFilePath = ""
	proxyQuestPeocessor *ProxyQuestProcessor
	proxyClient *ProxyClient

	session_mutex sync.Mutex
	sessionMap map[uint32]*SessionInfo
)

func ConfigFpnnTransfer(endpoint string, pemKeyFilePath string) {
	secondaryProxyServerEndpoint = endpoint
	secondaryProxyServerPemFilePath = pemKeyFilePath

	proxyQuestPeocessor = &ProxyQuestProcessor{}
	proxyClient = newProxyClient()
	sessionMap = make(map[uint32]*SessionInfo)
}

//=====================================================================//
//--                           Proxy Client                          --//
//=====================================================================//
func newProxyClient() *ProxyClient {
	client := &ProxyClient{}
	client.client = nil
	client.lastActiveTs = 0

	return client
}

func (proxyClient *ProxyClient) getClient() *fpnn.TCPClient {

	now := time.Now().Unix()

	proxyClient.mutex.Lock()
	defer proxyClient.mutex.Unlock()

	needCreate := false
	if proxyClient.lastActiveTs < now - 5 * 60 {
		needCreate= true
	} else if proxyClient.client == nil {
		needCreate = true
	}

	if needCreate {

		if proxyClient.client != nil {
			proxyClient.client.Close()
		}

		proxyClient.client = fpnn.NewTCPClient(secondaryProxyServerEndpoint)
		proxyClient.client.SetQuestProcessor(proxyQuestPeocessor)

		err := proxyClient.client.EnableEncryptor(secondaryProxyServerPemFilePath)
		if err != nil {
			fmt.Println("Enable encryptor for TCP client to server ", secondaryProxyServerEndpoint, " error: ", err)
		}
	}

	proxyClient.lastActiveTs = now

	return proxyClient.client
}

func newSessionInfo() *SessionInfo {
	info := new(SessionInfo)
	info.sendSeq = 0
	info.recvSeq = 0
	info.recvMap = make(map[uint32][]byte)

	return info
}

//=====================================================================//
//--                      ProxyQuestProcessor                        --//
//=====================================================================//

func (processor *ProxyQuestProcessor) Process(method string) func(*fpnn.Quest) (*fpnn.Answer, error) {
	if method == "httpsResponse" {
		return processor.httpsResponse
	} else {
		fmt.Println("Receive unknown method:", method)
		return nil
	}
}

func (processor *ProxyQuestProcessor) httpsResponse(quest *fpnn.Quest) (*fpnn.Answer, error) {

	sessionId := quest.WantUint32("sessionId")
	recvSeq := quest.WantUint32("seq")
	value := quest.WantString("data")
	data := []byte(value)
	
	session_mutex.Lock()
	sessionInfo, ok := sessionMap[sessionId]
	session_mutex.Unlock()

	if ok {
		sessionInfo.mutex.Lock()
		if recvSeq == sessionInfo.recvSeq + 1 {
			sessionInfo.conn.Write(data)
			sessionInfo.recvSeq = recvSeq

			for {
				cache, exist := sessionInfo.recvMap[sessionInfo.recvSeq + 1]
				if exist {
					sessionInfo.conn.Write(cache)
					delete(sessionInfo.recvMap, sessionInfo.recvSeq + 1)
					sessionInfo.recvSeq++
				} else {
					break
				}
			}

		} else {
			if recvSeq > sessionInfo.recvSeq + 1 {
				sessionInfo.recvMap[recvSeq] = data
			}	
		}
		sessionInfo.mutex.Unlock()
	}

	return fpnn.NewAnswer(quest), nil
}

//=====================================================================//
//--                            HTTP Hijack                          --//
//=====================================================================//

func convertToString(value interface{}, unconvertPanic bool) string {
	switch value.(type) {
	case string:
		return value.(string)
	case []byte:
		return string(value.([]byte))
	case []rune:
		return string(value.([]rune))
	default:
		if !unconvertPanic {
			return ""
		} else {
			panic("Type convert failed.")
		}
	}
}

func convertHeaderToStringArray(head http.Header) []string {
	
	var headerVector []string

	for k, vv := range head {
		for _, v := range vv {

			var builder strings.Builder

			if k != "Proxy-Connection" {
				builder.WriteString(k)
			} else {
				builder.WriteString("Connection")
			}
			
			builder.WriteString(": ")
			builder.WriteString(v)

			headerVector = append(headerVector, builder.String())
		}
	}

	return headerVector
}

func fpnnHandleHttp(w http.ResponseWriter, r *http.Request) {

	bodyBytes, _ := ioutil.ReadAll(r.Body)

	quest := fpnn.NewQuest("http")
	quest.Param("method", r.Method)
	quest.Param("url", r.URL.String())
	quest.Param("header", convertHeaderToStringArray(r.Header))
	quest.Param("body", string(bodyBytes))

	client := proxyClient.getClient()
	answer, err := client.SendQuest(quest, 120 * time.Second)

	if answer != nil {
		if answer.IsException() {
			code := answer.WantInt("code")
			ex := answer.WantString("ex")

			fmt.Println("Receive error answer for HTTP quest. Method: ", r.Method, ", Url: ", r.URL.String(), ". error code:", code, ", message:", ex)
			if err == nil {
				var builder strings.Builder
				builder.WriteString("error code: ")
				builder.WriteString(strconv.Itoa(code))
				builder.WriteString(", message: ")
				builder.WriteString(ex)

				http.Error(w, builder.String(), http.StatusServiceUnavailable)
			} else {
				http.Error(w, err.Error(), http.StatusServiceUnavailable)
			}

		} else {
			proxyClient.lastActiveTs = time.Now().Unix()

			code := answer.WantInt("httpCode")
			header := answer.WantMap("header")
			body := answer.WantString("body")

			for k, v := range header {
				key := convertToString(k, false)
				value := convertToString(v, false)

				w.Header().Add(key, value)
			}

			w.WriteHeader(code)
			w.Write([]byte(body))
			return
		}

	} else {
		fmt.Println("Send HTTP quest failed. Method: ", r.Method, ", Url: ", r.URL.String(), ". err: ", err)
		http.Error(w, err.Error(), http.StatusServiceUnavailable)
	}
}

//=====================================================================//
//--                            HTTPS Hijack                          --//
//=====================================================================//

func fpnnHandleHttps(w http.ResponseWriter, r *http.Request) {

	quest := fpnn.NewQuest("initHttps")
	quest.Param("host", r.Host)

	client := proxyClient.getClient()
	answer, err := client.SendQuest(quest, 120 * time.Second)
	
	if answer != nil {
		if answer.IsException() {
			code := answer.WantInt("code")
			ex := answer.WantString("ex")

			fmt.Println("Receive error answer for init HTTPS connection. Host: ", r.Host, ". error code:", code, ", message:", ex)
			if err == nil {
				var builder strings.Builder
				builder.WriteString("error code: ")
				builder.WriteString(strconv.Itoa(code))
				builder.WriteString(", message: ")
				builder.WriteString(ex)

				http.Error(w, builder.String(), http.StatusServiceUnavailable)
			} else {
				http.Error(w, err.Error(), http.StatusServiceUnavailable)
			}

		} else {
			proxyClient.lastActiveTs = time.Now().Unix()

			sessionId := answer.WantUint32("sessionId")

			w.WriteHeader(http.StatusOK)

			sessinInfo := newSessionInfo()
			sessinInfo.sessionId = sessionId

			hijacker, ok := w.(http.Hijacker)
			if !ok {
				http.Error(w, "Hijacking not supported", http.StatusInternalServerError)
				return
			}

			clientConn, _, err := hijacker.Hijack()
			sessinInfo.conn = clientConn
			if err != nil {
				http.Error(w, err.Error(), http.StatusServiceUnavailable)
				return
			}

			session_mutex.Lock()
			sessionMap[sessionId] = sessinInfo
			session_mutex.Unlock()

			go httpsTransfer(sessinInfo)
			return
		}

	} else {
		fmt.Println("Init https connection failed. Host: ", r.Host, ". err: ", err)
		http.Error(w, err.Error(), http.StatusServiceUnavailable)
	}
}

func httpsTransfer(sessinInfo *SessionInfo) {

	defer sessinInfo.conn.Close()

	defer func() {
		session_mutex.Lock()
		delete(sessionMap, sessinInfo.sessionId)
		session_mutex.Unlock()

		quest := fpnn.NewQuest("httpsClose")
		quest.Param("sessionId", sessinInfo.sessionId)

		client := proxyClient.getClient()
		client.SendQuest(quest, 120 * time.Second)

		proxyClient.lastActiveTs = time.Now().Unix()
	}()

	buffer := make([]byte, 1024)

	for {
		n, err := sessinInfo.conn.Read(buffer)
		if err != nil {
			if err != io.EOF {
				fmt.Println("Read source https connection failed. err: ", err)
			}
			return
		} else {
			if n == 0 {
				return
			}

			sessinInfo.mutex.Lock()
			sessinInfo.sendSeq++
			sendId := sessinInfo.sendSeq
			sessinInfo.mutex.Unlock()

			quest := fpnn.NewQuest("https")
			quest.Param("sessionId", sessinInfo.sessionId)
			quest.Param("data", buffer[:n])
			quest.Param("seq", sendId)

			client := proxyClient.getClient()
			answer, err := client.SendQuest(quest, 120 * time.Second)

			if answer != nil {
				if answer.IsException() {
					code := answer.WantInt("code")
					ex := answer.WantString("ex")

					fmt.Println("Transfer https data to server answer exception. size: ", len(buffer), ". error code:", code, ", message:", ex)
					return

				} else {
					proxyClient.lastActiveTs = time.Now().Unix()
					continue
				}

			} else {
				fmt.Println("Transfer https data to server failed. size: ", len(buffer), ". err: ", err)
				return
			}
		} 
	}
}
