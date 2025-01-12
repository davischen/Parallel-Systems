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
	"sync"
    "io/ioutil"
	"strconv"
	"strings"
)

type Tree struct {
	Left  *Tree
	Value int
	Right *Tree
}

type TreeHash struct {
	TreeId int
	Hash   int
}

func initializeAdjacencyMatrix(n int) AdjacencyMatrix {
	matrix := AdjacencyMatrix{n, make([]bool, n*(n+1)/2)}
	for i := range matrix.Values {
		matrix.Values[i] = false
	}
	return matrix
}

func inOrderTraverseAndCalculateHash(n *Tree, hash *int) {
    if n == nil {
        return
    }

    inOrderTraverseAndCalculateHash(n.Left, hash)

    *hash = calculateHash(*hash, n.Value)

    inOrderTraverseAndCalculateHash(n.Right, hash)
}

func calculateHash(hash int, value int) int {
    new_value := value + 2
    hash = (hash * new_value + new_value) % 1000
    return hash
}

func (tree *Tree) Hash(prev_hash int) int {
	if tree == nil {
		return prev_hash
	}
	hash := tree.Left.Hash(prev_hash)
	hash = (hash*(tree.Value+2) + (tree.Value + 2)) % 1000
	return tree.Right.Hash(hash)
}


func hash(tree *Tree) int {
	hash := 1
	inOrder(tree, func(value int) {
		newValue := value + 2
		hash = (hash * newValue + newValue) % 1000
	})
	return hash
}

func (tree *Tree) HashUsingInOrder() int {
    hash := 1 
    inOrderTraverseAndCalculateHash(tree, &hash)
    return hash
}

func insertUnsorted(group []int, treeId int) []int {
	group = append(group, treeId)
	return group
}

func inOrder(tree *Tree, visit func(int)) {
	if tree == nil {
		return
	}
	inOrder(tree.Left, visit)
	visit(tree.Value)
	inOrder(tree.Right, visit)
}

func treesEqualSequential(tree_1 *Tree, tree_2 *Tree) bool {
	var tree_1_stack []*Tree
	var tree_2_stack []*Tree

	tree_1_it := tree_1
	tree_2_it := tree_2

	for (len(tree_1_stack) != 0 || len(tree_2_stack) != 0) ||
		(tree_1_it != nil || tree_2_it != nil) {

		for ; tree_1_it != nil; tree_1_it = tree_1_it.Left {
			tree_1_stack = append(tree_1_stack, tree_1_it)
		}
		for ; tree_2_it != nil; tree_2_it = tree_2_it.Left {
			tree_2_stack = append(tree_2_stack, tree_2_it)
		}

		tree_1_it = tree_1_stack[len(tree_1_stack)-1]
		tree_1_stack = tree_1_stack[:len(tree_1_stack)-1]
		tree_2_it = tree_2_stack[len(tree_2_stack)-1]
		tree_2_stack = tree_2_stack[:len(tree_2_stack)-1]

		if tree_1_it.Value != tree_2_it.Value {
			return false
		}

		tree_1_it = tree_1_it.Right
		tree_2_it = tree_2_it.Right
	}

	if (len(tree_1_stack) != 0 || len(tree_2_stack) != 0) ||
		(tree_1_it != nil || tree_2_it != nil) {
		return false
	}

	return true
}

func ReadTrees(inputFile *string) []*Tree {
	var trees []*Tree
	content, err := ioutil.ReadFile(*inputFile)

	if err != nil {
		log.Fatal(err)
	}

	for _, row := range strings.Split(string(content), "\n") {
		if row == "" {
			break
		}
		var root *Tree
		for _, number_s := range strings.Split(string(row), " ") {
			number, err := strconv.ParseInt(number_s, 10, 32)
			if err != nil {
				log.Fatal(err)
			}
			if root == nil {
				root = &Tree{nil, int(number), nil}
			} else {
				insert(root, int(number))
			}
		}
		trees = append(trees, root)
	}
	return trees
}

func insert(tree *Tree, number int) {
	if number < tree.Value {
		if tree.Left == nil {
			tree.Left = &Tree{nil, number, nil}
		} else {
			insert(tree.Left, number)
		}
	} else {
		if tree.Right == nil {
			tree.Right = &Tree{nil, number, nil}
		} else {
			insert(tree.Right, number)
		}
	}
}


func hashTreesSequential(trees []*Tree, updateMap bool) HashGroups {
	var treesByHashGroup = make(HashGroups, 1000) 
	startTime := time.Now()
	for treeId, tree := range trees {
		hashValue := hash(tree) 

		if updateMap {
            treesByHashGroup[hashValue] = append(treesByHashGroup[hashValue], treeId)//insertSorted(treesByHashGroup[hashValue], treeId)
		}
	}
	elapsed := time.Since(startTime).Seconds()
	fmt.Printf("hashTime: %f\n", float64(elapsed))
	return treesByHashGroup
}




func compareTreesSequential(hashGroup HashGroups, trees []*Tree) ComparisonGroups {
	var comparisonGroups ComparisonGroups
	comparisonGroups.TreeMapping = make(map[int]AdjacencyMatrix)
	comparisonGroups.GroupIds = make([]bool, len(trees))

	for i := range comparisonGroups.GroupIds {
		comparisonGroups.GroupIds[i] = false
	}

	for hash, treeIndexes := range hashGroup {
        n := len(treeIndexes) 
		if n <= 1 {
			continue
		}

		matrix := initializeAdjacencyMatrix(n)

		for i, treeI := range treeIndexes {
			if comparisonGroups.GroupIds[treeI] {
					continue
		    }

		    for j, treeJ := range treeIndexes[i+1:] {
			    //fmt.Printf("Comparing trees %d and %d\n", treeI, treeJ) 

			    if !comparisonGroups.GroupIds[treeJ] && treesEqualSequential(trees[treeI], trees[treeJ]) {
						comparisonGroups.GroupIds[treeJ] = true
						matrix.Values[i*(2*n-i+1)/2+j+1] = true
                        //fmt.Printf("n: %d, index (%d, %d) = %d\n",n,i,j, (i*(2*n-i+1)/2+j+1))
						//fmt.Printf("Trees %d and %d are equal\n", treeI, treeJ) 
				}
			}
		}

		//fmt.Printf("Storing matrix for hash %d: %+v\n", hash, matrix.Values)
		comparisonGroups.TreeMapping[hash] = matrix
		
    }

	return comparisonGroups
}


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

type WorkBuffer struct {
	items     []*WorkBufferItem
	done      bool
	size      int
	lock      *sync.Mutex
	waitFull  *sync.Cond
	waitEmpty *sync.Cond
}

type WorkBufferItem struct {
	matrix *AdjacencyMatrix
	i      int
	treeI  int
	j      int
	treeJ  int
}

func initializeComparisonGroups(treeCount int) ComparisonGroups {
	return ComparisonGroups{
		TreeMapping: make(map[int]AdjacencyMatrix),
		GroupIds:    make([]bool, treeCount),
	}
}

func compareTreesParallel(hashGroup HashGroups, trees []*Tree, compWorkersCount int) ComparisonGroups {
	var wg sync.WaitGroup

	comparisonGroups := initializeComparisonGroups(len(trees))

	workChan := make(chan *WorkBufferItem, compWorkersCount*10)//compWorkersCount

	for i := 0; i < compWorkersCount; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for item := range workChan {
				if !comparisonGroups.GroupIds[item.treeI] &&
					treesEqualSequential(trees[item.treeI], trees[item.treeJ]) {
					comparisonGroups.GroupIds[item.treeJ] = true
					item.matrix.Values[item.i*(2*item.matrix.Size-item.i+1)/2+item.j+1] = true
				}
			}
		}()
	}

	for hash, treeIndexes := range hashGroup {
		n := len(treeIndexes)
		if n <= 1 {
			continue
		}

		matrix := initializeAdjacencyMatrix(n)

		for i, treeI := range treeIndexes {
			if comparisonGroups.GroupIds[treeI] {
				continue
			}

			for j, treeJ := range treeIndexes[i+1:] {
				workChan <- &WorkBufferItem{matrix: &matrix, i: i, treeI: treeI, j: j, treeJ: treeJ}
			}
		}
		comparisonGroups.TreeMapping[hash] = matrix
	}

	close(workChan)

	wg.Wait()

	return comparisonGroups
}



func compareTreesParallelUnbuffered(hashGroup HashGroups, trees []*Tree) ComparisonGroups {
	var wg sync.WaitGroup
	var mu sync.Mutex 
	var comparisonGroups ComparisonGroups
	comparisonGroups.TreeMapping = make(map[int]AdjacencyMatrix)
	comparisonGroups.GroupIds = make([]bool, len(trees))

	for i := range comparisonGroups.GroupIds {
		comparisonGroups.GroupIds[i] = false
	}

	for hash, treeIndexes := range hashGroup {
		n := len(treeIndexes) 
		if n <= 1 {
			continue
		}

		matrix := initializeAdjacencyMatrix(n)

		for i, treeI := range treeIndexes {
			if comparisonGroups.GroupIds[treeI] {
				continue
			}

			for j, treeJ := range treeIndexes[i+1:] {
				wg.Add(1)

				go func(matrix AdjacencyMatrix, n int, i int, treeI int, j int, treeJ int) {
					defer wg.Done()

					if treesEqualSequential(trees[treeI], trees[treeJ]) {
						mu.Lock() 
						if !comparisonGroups.GroupIds[treeI] {
							comparisonGroups.GroupIds[treeJ] = true
							matrix.Values[i*(2*n-i+1)/2+j+1] = true
						}
						mu.Unlock() 
					}
				}(matrix, n, i, treeI, j, treeJ)
			}
		}
		mu.Lock()
		comparisonGroups.TreeMapping[hash] = matrix
		mu.Unlock()
	}

	wg.Wait()

	return comparisonGroups
}



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