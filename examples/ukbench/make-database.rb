require 'rubygems'
require 'gdbm'

DBM_FILE = 'ukbench.gdbm'

File::open("./id.txt") do |f|
  db = GDBM.open(DBM_FILE, 0600)
  count = db['COUNT'] ? db['COUNT'].to_i : 0
  
  while line = f.gets
    line = line.chomp
    id, file = line.split(' ')
    unless (db[id])
      db[id] = file
      db[count.to_s] = id
      count += 1
    end
  end
  db['COUNT'] = count.to_s
  puts db['COUNT']
  db.close
end
