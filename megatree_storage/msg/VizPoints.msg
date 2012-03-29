VizPoint[] points
float64 min_cell_size
float64 max_cell_size
float64 root_size

geometry_msgs/Pose camera_pose

# id of the request
uint64 id

# how long did it take the viz_server to respond to the request 
duration viz_server_duration

# how long did it take the viz_server to retrieve the points from the tree
duration tree_query_duration

# Amount of time that compression took.
duration compression_duration

# the time when the request for points was sent from the client
time request_time
