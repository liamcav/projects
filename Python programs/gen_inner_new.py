#takes 

from music21 import *
import os
import sys
import re
import numpy as np

class Node:

	def __init__(self, id, s, a, t, b, edges):
		self.id = id
		self.s = s
		self.a = a
		self.t = t
		self.b = b
		self.edges = edges

		


#c-list of S and B in chorale, i-current index in c, g-graph, p-potential next nodes, l-list of paths to return	
def traverse(chord, c, i, g, depth):
	queue = [(chord, [[chord.id, 0]])]
	while queue:
		(node, path) = queue.pop(0)
		for next in node.edges:
			nl = [item for item in g if item.id==next[0]]
			n=nl[0]
			if len(path)<len(c):
				if n.s==c[len(path)].pitches[0].midi and n.b==c[len(path)].pitches[-1].midi:
					if len(path)+1 == len(c):
						yield path + [next]	
					else:
						queue.append((n, path + [next]))
						print(len(path))
						

					
if len(sys.argv) != 2:
	print("Usage: gen_inner.py graph")
	exit()	
	
#get graph	
f = open(sys.argv[1], "r")
lines = []
for line in f:
	lines.append(line)
nodes=[]
for line in lines:
	arr = line.split()
	id = int(arr[0])
	tid = id
	s = tid%127
	tid = tid//127
	a = tid%127
	tid = tid//127
	t = tid%127
	tid = tid//127
	b = tid%127	
	
	e = []
	for i, item in enumerate(arr):
		if i%2==1 and len(arr)>2:
			t=[]
			t.append(int(re.sub('[^0-9]', '', item)))
			t.append(int(re.sub('[^0-9]', '', arr[i+1])))
			e.append(t)	
	nodes.append(Node(id, s, a, t, b, e))	
	
#get S and B	
bci = corpus.chorales.Iterator()
s=bci.next()
s=bci.next()
s=bci.next()
s=bci.next()
s=bci.next()
s=bci.next()
s=bci.next()
s=bci.next()
s=bci.next()
k = s.analyze('key')
if k.mode == 'major':
	i = interval.Interval(k.tonic, pitch.Pitch('C'))
else:
	i = interval.Interval(k.tonic, pitch.Pitch('A'))
s = s.transpose(i)

s.write('midi', fp='4voice_trans_in.midi')
		
chordify = s.chordify()	
chords = chordify.recurse().getElementsByClass('Chord')
#shortening
chords = chords[:25]
#
durations=[chord.duration for chord in chords]
#durations=[]
#for chord in chords:
#	durations.append(chord.duration)
s = chords[0].pitches[0].midi
b = chords[0].pitches[-1].midi
potential_first=[]
for i in nodes:
	if i.b==b and i.s==s:
		potential_first.append(i)
paths = []
for item in potential_first:
	generate = list(traverse(item, chords, 0, nodes, 0))
	if len(generate)>0: paths.append(generate[0])
max_sum_index=-1
max_sum=0
print("Number of possible paths: ", len(paths))
for i,p in enumerate(paths):
	sum=0
	for j in p:
		sum+=j[1]
	if sum > max_sum:
		max_sum = sum 
		max_sum_index = i		
if len(paths) == 0:
	print("no match found")
	exit()
path = paths[max_sum_index]
stream_s = stream.Part()
stream_a = stream.Part()
stream_t = stream.Part()
stream_b = stream.Part()
for item in path:
	id = item[0]
	s=id%127
	id = id//127
	a=id%127
	id = id//127
	t=id%127
	id = id//127
	b=id%127
	stream_b.append(note.Note(b, type='quarter'))
	stream_t.append(note.Note(t, type='quarter'))
	stream_a.append(note.Note(a, type='quarter'))
	stream_s.append(note.Note(s, type='quarter'))


sc = stream.Score()
sc.insert(0,stream_s)
sc.insert(0,stream_a)
sc.insert(0,stream_t)
sc.insert(0,stream_b)
fp= sc.write('midi', fp='4voice_trans_out.midi')
sc.show('midi')
	
	
	
	
	