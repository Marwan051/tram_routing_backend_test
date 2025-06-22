package main

import (
	"back/test/routing"
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"
)

const PORT = 3000

func main() {

	mux := http.NewServeMux()
	mux.HandleFunc("GET /getPath", getPathHandler)

	server := &http.Server{
		Addr:    ":" + fmt.Sprint(PORT),
		Handler: mux,
	}

	go func() {
		log.Printf("Server starting on %d", PORT)
		if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatalf("server failed to start: %v", err)
		}
	}()

	// Wait for interrupt signal to gracefully shutdown
	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
	<-quit

	log.Println("Server shutting down...")

	// Graceful shutdown with timeout
	shutdownCtx, shutdownCancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer shutdownCancel()

	if err := server.Shutdown(shutdownCtx); err != nil {
		log.Fatalf("server forced to shutdown: %v", err)
	}

	log.Println("Server exited")
}

func getPathHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "unsupported method", http.StatusMethodNotAllowed)
		return
	}
	q := r.URL.Query()
	start := q.Get("start")
	end := q.Get("end")
	mode := q.Get("mode")

	if start == "" || end == "" || mode == "" {
		http.Error(w, "missing start, end or mode param", http.StatusBadRequest)
		return
	}

	// call your CLI wrapper, which returns JSON
	data, err := routing.GetRoute(start, end, mode)
	if err != nil {
		log.Printf("routing error: %v", err)
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	// ensure valid JSON
	if !json.Valid(data) {
		log.Printf("invalid JSON from CLI: %q", data)
		http.Error(w, "bad JSON from routing engine", http.StatusInternalServerError)
		return
	}
	// Unmarshal JSON into a map
	var result map[string]interface{}
	if err := json.Unmarshal(data, &result); err != nil {
		log.Printf("failed to unmarshal JSON: %v", err)
		http.Error(w, "failed to parse routing engine response", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.Write(data)
}
