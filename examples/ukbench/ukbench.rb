require 'rubygems'
require 'otama'
require 'yaml'
require 'gdbm'

def ukbench
  otama = Otama.open("./config.yaml")
  db = GDBM::open("./ukbench.gdbm", 0600)
  otama.pull
  
  begin
    otama.invoke("update_idf", 0)
  rescue
  end
  
  sum = 0
  count = 0
  qps = 0.0
  t = Time.now

  rdb = {}
  db.keys.each do |key|
    rdb[db[key]] = key
  end
  db.keys.each do |id|
    line = db[id]
    if (line =~ /ukbench(\d{5})/)
      no = $1.to_i / 4
      results = otama.search(4, :id => rdb[line])
      sum += results.reduce(0) do |s, v|
        rf = db[v.id]
        if (rf && rf =~ /ukbench(\d{5})/)
          rno = $1.to_i / 4
          if (no == rno)
            s + 1
          else
            s
          end
        else
          s
        end
      end
      count += 1
      qps = count / (Time.now - t)
    end
    printf("%5d (%.2f) %.4fqps\r", count, sum.to_f / count.to_f, qps)
  end
  printf("%5d/%5d (%.2f) %.4fqps\n", sum, count, sum.to_f / count.to_f, qps)  
  otama.close
  db.close
end

STDOUT.sync = true 
ukbench
