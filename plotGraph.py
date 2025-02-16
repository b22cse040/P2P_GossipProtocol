import networkx as nx
import matplotlib.pyplot as plt

G = nx.DiGraph()

# Read from graph.txt
with open("graph.txt", "r") as file:
    for line in file:
        parts = line.strip().split()
        if parts[0] == "Seed":
            G.add_node(f"Seed-{parts[1]}", color="red")
        elif parts[0] == "Peer":
            G.add_node(f"Peer-{parts[1]}", color="blue")
        elif parts[0] == "Connection":
            G.add_edge(f"Peer-{parts[1]}", f"Peer-{parts[3]}")

# Set colors
colors = [G.nodes[node]["color"] for node in G.nodes]

plt.figure(figsize=(8, 6))
pos = nx.spring_layout(G, seed=42)
nx.draw(G, pos, with_labels=True, node_size=3000, node_color=colors, font_size=10, font_weight="bold")
plt.title("P2P Gossip Network Graph")
plt.show()
