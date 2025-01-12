package main

import (
	"sync"
)

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

