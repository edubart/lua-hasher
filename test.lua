local hasher = require 'hasher'
assert(type(hasher) == 'table')
assert(hasher.VERSION == 'hasher 0.1.0')
assert(hasher.blake2b)

do
  print("testing base58...")
  -- encode
	assert(hasher.base58encode('') == '')
	assert(hasher.base58encode('\0\0') == '11')
	assert(hasher.base58encode('o hai') == 'DYB3oMS')
	assert(hasher.base58encode('Hello world') == 'JxF12TrwXzT5jvT')
	-- decode
	assert(hasher.base58decode('') == '')
	assert(hasher.base58decode('11') == '\0\0')
	assert(hasher.base58decode('DYB3oMS') == 'o hai')
	assert(hasher.base58decode('JxF12TrwXzT5jvT') == 'Hello world')
end

do
	print("testing blake2b...")
  local s = "The quick brown fox jumps over the lazy dog"
  assert(hasher.base58encode(hasher.blake2b(s, 1)) ==
    "48")
  assert(hasher.base58encode(hasher.blake2b(s, 32)) ==
    "6dqGv62vgEFmtXqP4ktHtLfbE7F89x76XNZXH35YjQt")
  assert(hasher.base58encode(hasher.blake2b(s)) ==
    "4NbsMDFzkkPtsm4Cz3jrdQN1RL7hhZMLg8s69hbGnnBssfKf6qeX6JepguVeRiumaf3foT7USYLzX2CTvRB3drWb")

  local key = 'my testing key'
  assert(hasher.base58encode(hasher.blake2b(s, 1, key)) ==
    "36")
  assert(hasher.base58encode(hasher.blake2b(s, 32, key)) ==
    "DJmvLDo6UHq5hbUkXco7ECUASnkDpFR2GHQigSqtTXKw")
  assert(hasher.base58encode(hasher.blake2b(s, 64, key)) ==
    "498ceyJGHVEYy9h2XJ9g6e4iKiU7btNyCRXiFGsc5pxLsTJwuStzoXx2qvN8QRXeJMH15Zur6LWxMcDSt2GnTQJF")
end
