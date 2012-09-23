require 'rubygems'
require 'otama'
require 'yaml'
require 'gdbm'

if (ARGV.length == 0)
  warn "usage: $0 files.."
  exit(-1)
end
  
CONFIG = './config.yaml'
DBM_FILE = 'example.gdbm'

Otama.open(YAML.load_file(CONFIG)) do |otama|
  db = GDBM::open(DBM_FILE, 0600)
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
  db.close
end

