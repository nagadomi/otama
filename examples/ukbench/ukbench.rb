require 'rubygems'
require 'otama'
require 'yaml'
require 'gdbm'

def ukbench
  file = File.open("filelist.txt")
  otama = Otama.open("./config.yaml")
  db = GDBM::open("./ukbench.gdbm", 0600)
  otama.pull
  
  begin
    otama.set("update_idf", 0)
  rescue
  end
  
  sum = 0
  count = 0
  qps = 0.0
  t = Time.now
  
  until (file.eof?)
    line = file.readline.chomp
    if (line =~ /ukbench(\d{5})/)
      no = $1.to_i / 4
      results = otama.search(4, :file => line)
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
    printf("%5d (%.4f) %.4fqps\r", count, sum.to_f / count.to_f, qps)
  end
  printf("%5d/%5d (%.4f) %.4fqps\n", sum, count * 4, sum.to_f / count.to_f, qps)  
  file.close
  otama.close
  db.close
end

STDOUT.sync = true 
ukbench
