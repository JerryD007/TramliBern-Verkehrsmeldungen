from pathlib import Path
s=(Path(__file__).parents[1]/"RundeUhr/IssNetwork.h").read_text()
assert s.index("http.begin(client") < s.index("client.connectVerified") < s.index("http.GET()")
print("PASS HTTP initialization precedes verified ISS connection")
