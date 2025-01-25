package main

import (
	"sync"
	"time"
	"fmt"
)

func min(a int, b int) int {
	if a < b {
		return a
	} else {
		return b
	}
}

type HashGroup struct {
    potTwins []int
    lock     *sync.Mutex
}

func hashTreesParallel_simple(trees []*Tree, hashWorkersCount int, updateMap bool) HashGroups {
    hashGroups := make(HashGroups,  1000) 
    var wg sync.WaitGroup 

    hashBatchSize := len(trees) / hashWorkersCount + (min(len(trees)%hashWorkersCount, 1))//(len(trees) + hashWorkersCount - 1) / hashWorkersCount 

    if hashBatchSize == 0 {
        hashBatchSize = 1
        hashWorkersCount = len(trees)
    }
    hashLocks := make([]sync.Mutex, 128)
	startTime := time.Now()
    for i := 0; i < hashWorkersCount; i++ {
        wg.Add(1)
        go func(wkIndex int) {
            defer wg.Done()
            startIdx := min(wkIndex * hashBatchSize, len(trees))
            endIdx := min(startIdx+hashBatchSize, len(trees))

            for treeIdx := startIdx; treeIdx < endIdx; treeIdx++ {
                if updateMap {
                    hashValue := trees[treeIdx].HashUsingInOrder()//.Hash(1) 
                    hashLocks[hashValue%len(hashLocks)].Lock()
                    groupIndex := hashValue % len(hashGroups)
                    hashGroups[groupIndex] = append(hashGroups[groupIndex], treeIdx) 
                    hashLocks[hashValue%len(hashLocks)].Unlock()
                } else{
                    _ = trees[treeIdx].HashUsingInOrder()//Hash(1)
                }
            }
        }(i)
    }

    wg.Wait() 
	elapsed := time.Since(startTime).Seconds()
	fmt.Printf("hashTime: %f\n", float64(elapsed))
    return hashGroups
}

func hashTreesParallel_fineGrainSync_noChannel(trees []*Tree, hashWorkersCount int, dataWorkersCount int) HashGroups {
    hashGroups := make(HashGroups, 1000) 
    var wg sync.WaitGroup
    hashBatchSize := len(trees) / hashWorkersCount + (min(len(trees)%hashWorkersCount, 1))

    if hashBatchSize == 0 {
        hashBatchSize = 1
        hashWorkersCount = len(trees)
    }

    
    //hashLocks := make([]sync.Mutex, 1000)
	hashLocks := make([]sync.Mutex, 128)
	startTime := time.Now()
    for i := 0; i < hashWorkersCount; i++ {
        wg.Add(1)
        go func(wk_index int) {
            defer wg.Done()
            startIdx := min(wk_index*hashBatchSize, len(trees))
            endIdx := min((wk_index+1)*hashBatchSize, len(trees))

            for treeIdx := startIdx; treeIdx < endIdx; treeIdx++ {
                hashValue := trees[treeIdx].HashUsingInOrder()//Hash(1) //.

                
				hashLocks[hashValue%len(hashLocks)].Lock()
                hashGroups[hashValue] = append(hashGroups[hashValue], treeIdx)
                hashLocks[hashValue%len(hashLocks)].Unlock()
            }
        }(i)
    }

    wg.Wait() 
	elapsed := time.Since(startTime).Seconds()
	fmt.Printf("hashTime: %f\n", float64(elapsed))
    return hashGroups
}



//==========================================================
func hashTreesParallel_fineGrainSync(trees []*Tree, hashWorkersCount int, dataWorkersCount int,
	buffered bool) HashGroups {
    hashGroups := make(HashGroups, 1000) 
    var wg sync.WaitGroup
    hashBatchSize := len(trees) / hashWorkersCount+ min(len(trees)%hashWorkersCount, 1)

    if hashBatchSize == 0 {
        hashBatchSize = 1
        hashWorkersCount = len(trees)
    }

    //hashLocks := make([]sync.Mutex, 128)
    
    var bufferSize int
    if buffered {
        bufferSize = len(trees) 
    } else {
        bufferSize = 0 
    }

    hashChan := make(chan struct {
        treeIdx   int
        hashValue int
    }, bufferSize)
	startTime := time.Now()
    for i := 0; i < hashWorkersCount; i++ {
        wg.Add(1)
        go func(wk_index int) {
            defer wg.Done()
            startIdx := min(wk_index*hashBatchSize, len(trees))
            endIdx := min((wk_index+1)*hashBatchSize, len(trees))

            for treeIdx := startIdx; treeIdx < endIdx; treeIdx++ {
                hashValue := trees[treeIdx].HashUsingInOrder()//Hash(1)
                hashChan <- struct {
                    treeIdx   int
                    hashValue int
                }{treeIdx, hashValue}
            }
        }(i)
    }

    go func() {
        wg.Wait()
        close(hashChan)
    }()

	elapsed := time.Since(startTime).Seconds()

    var dataWg sync.WaitGroup
    for i := 0; i < dataWorkersCount; i++ {
        dataWg.Add(1)
        go func() {
            defer dataWg.Done()
            for item := range hashChan {
                hashValue := item.hashValue
                treeIdx := item.treeIdx

                hashGroups[hashValue% len(hashGroups)] = append(hashGroups[hashValue% len(hashGroups)], treeIdx)
            }
        }()
    }

    dataWg.Wait()
	fmt.Printf("hashTime: %f\n", float64(elapsed))
    return hashGroups
}



// *hashWorkersCount > 1 && *dataWorkersCount > 0
// no lock
func hashTreesParallel_WithImmediateProcessing(trees []*Tree, hashWorkersCount int, dataWorkersCount int, buffered bool) HashGroups {
	//print("===hashTreesParallel_WithImmediateProcessing===\n")
	var hashGroups = make(HashGroups, 1000) 
	var hashWg sync.WaitGroup                
	hashBatchSize := len(trees) / hashWorkersCount + min(len(trees)%hashWorkersCount, 1)

	var TreeHashes []chan TreeHash       
	var localhashGroups = make([]HashGroups, hashWorkersCount) 
	
	hashLocks := make([]sync.Mutex, 128)

	TreeHashes = make([]chan TreeHash, dataWorkersCount)
	for j := range TreeHashes {
		if buffered {
			TreeHashes[j] = make(chan TreeHash, hashWorkersCount*10)
		} else {
			TreeHashes[j] = make(chan TreeHash)
		}
	}
	startTime := time.Now()
	var dataWg sync.WaitGroup
	for j := 0; j < dataWorkersCount; j++ {
		dataWg.Add(1)

		go func(j int) {
			defer dataWg.Done()
			for TreeHash := range TreeHashes[j] {
				hashLocks[TreeHash.Hash % len(hashLocks)].Lock()
				hashGroups[TreeHash.Hash] = append(hashGroups[TreeHash.Hash], TreeHash.TreeId)
				hashLocks[TreeHash.Hash % len(hashLocks)].Unlock()
			}
		}(j)
	}

	for i := 0; i < hashWorkersCount; i++ {
		hashWg.Add(1)

		go func(i int) {
			defer hashWg.Done()

			start := min(i*hashBatchSize, len(trees))
			end := min((i+1)*hashBatchSize, len(trees))

			hashWorker_channel(trees, start, end, localhashGroups[i], TreeHashes)
		}(i)
	}

	hashWg.Wait()
	for j := range TreeHashes {
		close(TreeHashes[j])
	}

	elapsed := time.Since(startTime).Seconds()
	dataWg.Wait()

	for _, local := range localhashGroups {
		mergeHashGroups(hashGroups, local)
	}


	fmt.Printf("hashTime: %f\n", float64(elapsed))
	return hashGroups
}

func hashWorker_channel(trees []*Tree, start int, end int, localTreesByHash HashGroups,TreeHashes []chan TreeHash) {
    for i, tree := range trees[start:end] {
        treeId := i + start
        hash := tree.Hash(1)

        if TreeHashes != nil {
            TreeHashes[hash%len(TreeHashes)] <- TreeHash{treeId, hash}
        } else {
            localTreesByHash[hash] = append(localTreesByHash[hash], treeId)
        }
    }
}

func mergeHashGroups(global HashGroups, local HashGroups) {
    for hash, ids := range local {
        global[hash] = append(global[hash], ids...)
    }
}


func hashWorker(trees []*Tree, start int, end int, treesByHash HashGroups,
	TreeHashes []chan TreeHash, globalHashLock []sync.Mutex) {
	for i, tree := range trees[start:end] {
		treeId := i + start
		hash := tree.Hash(1)

		if TreeHashes != nil {
			// send it to the worker according to hash
			TreeHashes[hash%len(TreeHashes)] <- TreeHash{treeId, hash}
		} else if globalHashLock != nil {
			globalHashLock[hash%len(globalHashLock)].Lock()
			treesByHash[hash] = append(treesByHash[hash], treeId)
			globalHashLock[hash%len(globalHashLock)].Unlock()
		}
	}
}