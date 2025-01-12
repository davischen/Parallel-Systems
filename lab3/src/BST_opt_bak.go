//TODO You should submit your code and report packaged into a tar file on Canvas. If you do not complete the optional implementation in step 2.2, name your main go program BST.go. If you do complete it, name it BST_opt.go instead.

package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"runtime/pprof"
	"time"
	"sort"
)

type HashGroups [][]int

func (hashGroups *HashGroups) Print() {
	//return
    //print("Print hashGroups:\n")
	for hash, treeIndexes := range *hashGroups {
		if len(treeIndexes) > 1 {
			fmt.Printf("%v:", hash)
			for _, index := range treeIndexes {
				fmt.Printf(" %v", index)
			}
			fmt.Printf("\n")
		}
	}
}

type ComparisonGroups struct {
	TreeMapping map[int]AdjacencyMatrix
	GroupIds  []bool
}

// triangular matrix array of size Size*(Size+1)/2
type AdjacencyMatrix struct {
	Size   int
	Values []bool
}

func (comparisonGroups *ComparisonGroups) Print(hashGroups *HashGroups) {
    //fmt.Println("Printing comparison groups without sorting...")

    //print("Print hashGroups...\n")
    group := 0
	//return
    for hash, matrix := range comparisonGroups.TreeMapping {
        n := matrix.Size
        treeIndexes := (*hashGroups)[hash]

        for i := 0; i < n; i++ {
            //print(treeIndexes[i])
            if comparisonGroups.GroupIds[treeIndexes[i]] {
                continue
            }

            var currentGroup []int

            for j := i + 1; j < n; j++ {
                if matrix.Values[i*(2*n-i+1)/2+j-i] {
                    currentGroup = append(currentGroup, treeIndexes[j])
                    comparisonGroups.GroupIds[treeIndexes[j]] = true
                }
            }

            if len(currentGroup) > 0 {
                currentGroup = append(currentGroup, treeIndexes[i])
                fmt.Printf("group %v:", group)
                for _, idx := range currentGroup {
                    fmt.Printf(" %v", idx)
                }
                fmt.Println()
                group++
            }
        }
    }
    //fmt.Println("Finished printing comparison groups.")
}

func (comparisonGroups *ComparisonGroups) PrintSorted(hashGroups *HashGroups) {
    fmt.Println("Printing comparison groups...")
    group := 0

    var sortedHashes []int
    for hash := range comparisonGroups.TreeMapping {
        sortedHashes = append(sortedHashes, hash)
    }
    sort.Ints(sortedHashes) 

    for _, hash := range sortedHashes {
        matrix := comparisonGroups.TreeMapping[hash]
        n := matrix.Size
        treeIndexes := (*hashGroups)[hash]

        sort.Ints(treeIndexes)

        for i := 0; i < n; i++ {
            if comparisonGroups.GroupIds[treeIndexes[i]] {
                continue
            }

            //printedGroup := false
            var currentGroup []int

            for j := i + 1; j < n; j++ {
                if matrix.Values[i*(2*n-i+1)/2+j-i] {
                    currentGroup = append(currentGroup, treeIndexes[j])
                    comparisonGroups.GroupIds[treeIndexes[j]] = true
                }
            }

            if len(currentGroup) > 0 {
                currentGroup = append(currentGroup, treeIndexes[i])
                sort.Ints(currentGroup) 
                fmt.Printf("group %v:", group)
                for _, idx := range currentGroup {
                    fmt.Printf(" %v", idx)
                }
                fmt.Println()
                group++
            }
        }
    }
    fmt.Println("Finished printing comparison groups.")
}

func main() {
    hashWorkersCount := flag.Int("hash-workers", 1, "number of goroutines to compute hash values")
    dataWorkersCount := flag.Int("data-workers", 0, "number of goroutines to handle data processing")
    compWorkersCount := flag.Int("comp-workers", 0, "number of goroutines for comparison tasks")
    inputFile := flag.String("input", "", "path to the input file containing tree data")
    // dataUseChannels := flag.Bool("data-use-channels", false, "enable fine-grained control using channels when data-workers are fewer than hash-workers")
    dataBuffered := flag.Bool("data-buffered", true, "enable buffered channel for data-worker writes")
    compBuffered := flag.Bool("comp-buffered", true, "use a custom work queue for comparison tasks by comp-workers")
    cpuprofile := flag.String("cpuprofile", "", "file path to save CPU profiling information")
    detailed := flag.Bool("detailed", true, "suppress printing of detailed results")

	flag.Parse()

	if *inputFile == "" {
		log.Fatal("input file cannot be blank!")
	}

	if *cpuprofile != "" {
		f, err := os.Create(*cpuprofile)
		if err != nil {
			log.Fatal(err)
		}
		pprof.StartCPUProfile(f)
		defer pprof.StopCPUProfile()
	}

	trees := ReadTrees(inputFile)

	start := time.Now()

	var hashGroups HashGroups
	if *hashWorkersCount <=1 {
			//print("*hashWorkersCount == 1 && *dataWorkersCount == 1 Done\n")
			//-hash-workers=1 -data-workers=1: This is the sequential implementation. Your program can compute the hash of all BSTs and update the map in the main thread.
            hashGroups = hashTreesSequential(trees, *dataWorkersCount == 1)
	} 
	//-data-workers=<number of workers to update the map>: there will be data-workers goroutines trying to update the map.
	if *hashWorkersCount > 1 && *dataWorkersCount == 0 { 
			hashGroups = hashTreesParallel_simple(trees, *hashWorkersCount, false)
	}
	//-hash-workers=i -data-workers=i(i>1): 
	//This implementation spawns i goroutines to compute the hashes of the input BSTs. Each goroutine updates the map individually after acquiring the mutex.
	if *hashWorkersCount > 1 && *dataWorkersCount == *hashWorkersCount && *dataWorkersCount >1 { 	
			hashGroups = hashTreesParallel_fineGrainSync_noChannel(trees, *hashWorkersCount, *dataWorkersCount)
	}
	//-hash-workers=i -data-workers=1(i>1): 
	// This implementation spawns i goroutines to compute the hashes of the input BSTs. Each goroutine sends its (hash, BST ID) pair(s) to a central manager goroutine using a channel. The central manager updates the map.
	if *hashWorkersCount > 1 && *dataWorkersCount == 1 { 
			hashGroups = hashTreesParallel_fineGrainSync(trees, *hashWorkersCount, *dataWorkersCount, *dataBuffered)
	}
	//-hash-workers=i -data-workers=j(i>j>1): This implementation spawns i goroutines to compute the hashes of the input BSTs. Then j goroutines are spawned to update the map.
	//It is up to you how the hash workers are communicating with the j data workers. You can use a semaphore so that j of the i hash workers can update the map. Or you can use j central managers to collect (hash, BST ID) pairs from each hash worker via j channels.
	//When j goroutines try to update the map, make sure each hash entry is not updated by more than two threads at the same time.
	if *hashWorkersCount > 1 && *dataWorkersCount != *hashWorkersCount && *dataWorkersCount > 1{ 
		hashGroups = hashTreesParallel_WithImmediateProcessing(trees, *hashWorkersCount, *dataWorkersCount, *dataBuffered)
	}

	elapsed := time.Since(start).Seconds()

    //print(len(trees))
	if *dataWorkersCount > 0 {
		fmt.Printf("hashGroupTime: %f\n", float64(elapsed))
	}
	if *detailed {
		hashGroups.Print()
	}
	if *compWorkersCount == 0 {
		return
	}

    
	start = time.Now()

	var comparisonGroups ComparisonGroups

	if *compWorkersCount == 1 {
		comparisonGroups = compareTreesSequential(hashGroups, trees)
	} else if *compBuffered {
		comparisonGroups = compareTreesParallel(hashGroups, trees, *compWorkersCount)
	} else {
		comparisonGroups = compareTreesParallelUnbuffered(hashGroups, trees)
	}

	elapsed = time.Since(start).Seconds()

	fmt.Printf("compareTreeTime: %f\n", float64(elapsed))
	if *detailed {
		comparisonGroups.Print(&hashGroups)
	}
}