import networkx as nx
import matplotlib.pyplot as plt


graph_file = "graph.txt"
G = nx.Graph()

alive_peers = set()
dead_peers = set()
seed_nodes = set()
seed_connections = []

with open(graph_file, "r") as file:
    for line in file:
        parts = line.strip().split()
        if not parts:
            continue
        if parts[0] == "Seed" and parts[1] != "Connection":
            seed_nodes.add(parts[1])
            G.add_node(parts[1])
        elif parts[0] == "Peer":
            alive_peers.add(parts[1])
            G.add_node(parts[1])
        elif parts[0] == "Connection":
            G.add_edge(parts[1], parts[3])
        elif parts[0] == "Seed" and parts[1] == "Connection":
            seed_connections.append((parts[2], parts[3]))
            G.add_edge(parts[2], parts[3])
        elif parts[0] == "[DEAD]":
            dead_peer = parts[2]
            dead_peers.add(dead_peer)



node_colors = []
for node in G.nodes():
    if node in dead_peers:
        node_colors.append("red")  # Dead peers in red
    elif node in seed_nodes:
        node_colors.append("green")  # Seed nodes in green
    else:
        node_colors.append("blue")  # Alive peers in blue

# Plot the graph
plt.figure(figsize=(12, 8))
pos = nx.spring_layout(G, seed=42)  
nx.draw(
    G, pos, with_labels=True, node_size=2500, node_color=node_colors,
    edge_color="gray", linewidths=1.5, font_size=14, font_color="white",
    font_weight="bold", arrows=False
)

nx.draw_networkx_edges(G, pos, edgelist=seed_connections, edge_color="orange", width=2, style="dashed")


plt.title("Peer-to-Peer Network Graph with Seed Connections and Dead Nodes", fontsize=16, fontweight="bold")
plt.show()
