package main

import (
	"io/ioutil"
	"log"
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
