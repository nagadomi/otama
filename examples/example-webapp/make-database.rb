require 'rubygems'
require 'otama'

DBM_FILE = './data/example.ldb'

unless defined?(Otama::KVS)
  warn "otama does not have KVS API."
  warn "please re-configure with --enable-leveldb option."
  exit(1)
end

File.open("./id.txt") do |f|
  Otama::KVS.open(DBM_FILE) do |db|
    count = db['COUNT'] ? db['COUNT'].to_i : 0
    while line = f.gets
      id, file = line.chomp.split(' ', 2)
      unless (db[id])
        db[id] = file
        db[count.to_s] = id
        count += 1
      end
    end
    db['COUNT'] = count.to_s
    puts "#{db['COUNT']} records"
  end
end
