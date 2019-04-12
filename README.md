# Hasher
A small library with hash and encoding functions implemented C. Currently supports
blake2b hashing and base58 encode/decode.

## Installation

```bash
luarocks install hasher
```

## Usage

Example usage:

```lua
local hasher = require 'hasher'

local binaryhash = hasher.blake2b('hello world')
local encodedhash = hasher.base58encode(binaryhash)
print(encodedhash)
-- outputs 3T7dTeZe2QgKBA8gHJS9uRPA2jAMGpNJewHaXaLhjeyV5GcMJnvjfW9FPrCvX7dVoN4qMGnQPTAX91UFVnAUW4f
```

### License
MIT
