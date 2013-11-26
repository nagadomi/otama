require 'rubygems'
require 'otama'

unless defined?(Otama::KVS)
  warn "otama does not have KVS API."
  warn "please re-configure with --enable-leveldb option."
  exit(1)
end

if (ARGV.length == 0)
  warn "usage: $0 files.."
  exit(-1)
end
  
CONFIG = './config.yaml'
DBM_FILE = './data/example.ldb'

Otama.open(CONFIG) do |otama|
  Otama::KVS.open(DBM_FILE) do |db|
    count = db['COUNT'] ? db['COUNT'].to_i : 0
    ARGV.each do |file|
      id = otama.insert(:file => file)
      unless (db[id])
        puts "#{id} #{file}"
        db[id] = file
        db[count.to_s] = id
        count += 1
      end
    end
    db['COUNT'] = count.to_s
    puts db['COUNT']
  end
end

