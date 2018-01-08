#Liam Caveney
#discription at: http://web.uvic.ca/~lcaveney/projects.html
#generates adjacency list for graph that represents theory rules
#input - midi file of chorale

from music21 import *
import os
import networkx as nx
import matplotlib.pyplot as plt

class Node:

	def __init__(self, id, s, a, t, b):
		self.id = id
		self.s = s
		self.a = a
		self.t = t
		self.b = b
		self.edges = []
		
	def add_edge(self, nid):
		if len(self.edges)==0:
			self.edges.append([nid, 1])	
		else:	
			for i in self.edges:
				if i[0] == nid:
					i[1] = i[1]+1	
					return;
			self.edges.append([nid, 1])	
	
chords = [] 
for filename in os.listdir('./chorales'):
	mf = midi.MidiFile()
	mf.open('./chorales/%s' % filename)
	mf.read()
	mf.close()
	s = midi.translate.midiFileToStream(mf)
	k = s.analyze('key')
	if k.mode == 'major':
		i = interval.Interval(k.tonic, pitch.Pitch('C'))
	else:
		i = interval.Interval(k.tonic, pitch.Pitch('A'))
	s = s.transpose(i)	
	curr_chordify = s.chordify()	
	curr_chords = curr_chordify.recurse().getElementsByClass('Chord')
	curr_chord_sequence = []
	curr_chord_nodes = []
	#create sequence and nodes of chords
	for c in curr_chords:
		a = []
		for p in c.pitches:
			a.append(p.midi)
		if len(a)==1: a.append(a[0])
		if len(a)==2: a.append(a[1])	
		if len(a)==3: a.append(a[2])	#doubles tenor with bass  what to do if same note???????
		f = a[0] + a[1]*127 + a[2]*127*127 + a[3]*127*127*127
		curr_chord_sequence.append(f)
		dup=0
		for i in chords:
			if i.id == f:
				dup=1
				break
		if dup==0:
			curr_chord_nodes.append(Node(f, a[3], a[2], a[1], a[0]))
			chords.append(Node(f, a[3], a[2], a[1], a[0]))
	#add edges from chord sequence
	for i, c in enumerate(curr_chord_sequence[:-1]):
		curr_c = curr_chord_sequence[i]
		next_c = curr_chord_sequence[i+1]
		node = [i for i in chords if i.id == curr_c]
		node[0].add_edge(next_c)

	

g = nx.DiGraph()
for c in chords:
	for e in c.edges:
		g.add_edge(c.id, e[0])
pos=nx.spring_layout(g)
nx.draw_networkx_nodes(g,pos,node_size=10)
nx.draw_networkx_edges(g,pos,width=0.01)
#nx.draw_networkx_labels(g,pos,font_size=5,font_family='sans-serif')
plt.axis('off')
plt.show()

for c in chords:
	print (c.id, c.edges)
