package main

import (
	"fmt"
	"time"
)

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
