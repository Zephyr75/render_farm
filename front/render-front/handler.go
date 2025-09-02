package function

import (
	_ "embed"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"math/rand"
	"net/http"
	"os"
	"sync"
	"time"
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
	ImgData string `json:"img_data"` // base64-encoded data URL
}

var (
	jobResults = make(map[string][]RenderResult)
	jobStatus  = make(map[string]bool)
	jobMutex   sync.Mutex
)

func Handle(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet && r.URL.Query().Has("job") {
		// Return job status
		jobID := r.URL.Query().Get("job")
		jobMutex.Lock()
		defer jobMutex.Unlock()

		if !jobStatus[jobID] {
			w.Header().Set("Content-Type", "application/json")
			json.NewEncoder(w).Encode(map[string]interface{}{
				"status": "pending",
			})
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"status": "complete",
			"images": jobResults[jobID],
		})
		return
	}

	if r.Method == http.MethodGet {
		// Serve frontend
		w.Header().Set("Content-Type", "text/html")
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

	// Generate a random job ID
	jobID := fmt.Sprintf("job-%d", rand.Intn(1000000))

	// Start rendering in background
	go renderJob(jobID, req)

	// Respond with job ID
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"job": jobID,
	})
}

func renderJob(jobID string, req RenderRequest) {
	const (
		s1y = 20
		s1z = 50
		s2x = 60
		s2y = 30
		s2z = 80
	)

	client := http.Client{Timeout: 10 * time.Minute}
	results := []RenderResult{}

	for s1x := req.S1XStart; s1x <= req.S1XEnd; s1x++ {
		url := fmt.Sprintf(
			os.Getenv("BACK_URL")+"/render?samples=%d&s1x=%d&s1y=%d&s1z=%d&s2x=%d&s2y=%d&s2z=%d",
			req.Samples, s1x, s1y, s1z, s2x, s2y, s2z,
		)

		resp, err := client.Get(url)
		if err != nil || resp.StatusCode != 200 {
			continue
		}
		imgData, _ := io.ReadAll(resp.Body)
		resp.Body.Close()

		dataURL := "data:image/png;base64," + base64.StdEncoding.EncodeToString(imgData)
		results = append(results, RenderResult{S1X: s1x, ImgData: dataURL})
	}

	jobMutex.Lock()
	jobResults[jobID] = results
	jobStatus[jobID] = true
	jobMutex.Unlock()
}
