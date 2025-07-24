package function

import (
	_ "embed"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"sync"
)

//go:embed index.html
var htmlContent string

type RenderRequest struct {
	S1XStart int `json:"s1x_start"`
	S1XEnd   int `json:"s1x_end"`
	Samples  int `json:"samples"`
}

type RenderResult struct {
	S1X     int    `json:"s1x"`
	ImgData string `json:"img_data"` // base64-encoded or data URL
}

func Handle(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet {
		// Serve frontend HTML
		w.Header().Set("Content-Type", "text/html")
		w.WriteHeader(http.StatusOK)
		fmt.Fprint(w, htmlContent)
		return
	}

	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req RenderRequest
	body, _ := io.ReadAll(r.Body)
	_ = json.Unmarshal(body, &req)

	// Fixed values from original script
	const (
		s1y = 20
		s1z = 50
		s2x = 60
		s2y = 30
		s2z = 80
	)

	var wg sync.WaitGroup
	var mu sync.Mutex
	results := []RenderResult{}

	for s1x := req.S1XStart; s1x <= req.S1XEnd; s1x++ {
		wg.Add(1)
		go func(s1x int) {
			defer wg.Done()

			url := fmt.Sprintf(
				"http://render.openfaas-fn:8080/render?samples=%d&s1x=%d&s1y=%d&s1z=%d&s2x=%d&s2y=%d&s2z=%d",
				req.Samples, s1x, s1y, s1z, s2x, s2y, s2z,
			)

			resp, err := http.Get(url)
			if err != nil || resp.StatusCode != 200 {
				return
			}
			defer resp.Body.Close()
			imgData, _ := io.ReadAll(resp.Body)

			// Embed as data URL (you could base64 it if needed)
			imgURL := "data:image/png;base64," + encodeBase64(imgData)

			mu.Lock()
			results = append(results, RenderResult{S1X: s1x, ImgData: imgURL})
			mu.Unlock()
		}(s1x)
	}

	wg.Wait()
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(results)
}

func encodeBase64(data []byte) string {
	return fmt.Sprintf("%s", toBase64(data))
}

func toBase64(data []byte) []byte {
	encoding := "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
	dst := make([]byte, ((len(data)+2)/3)*4)
	for i, j := 0, 0; i < len(data); i += 3 {
		var b [3]byte
		copy(b[:], data[i:])
		dst[j+0] = encoding[b[0]>>2]
		dst[j+1] = encoding[((b[0]&0x03)<<4)|(b[1]>>4)]
		if i+1 < len(data) {
			dst[j+2] = encoding[((b[1]&0x0F)<<2)|(b[2]>>6)]
		} else {
			dst[j+2] = '='
		}
		if i+2 < len(data) {
			dst[j+3] = encoding[b[2]&0x3F]
		} else {
			dst[j+3] = '='
		}
		j += 4
	}
	return dst
}
