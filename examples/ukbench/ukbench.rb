require 'rubygems'
require 'otama'

CONFIG_FILE = "./config.yaml"
DBM_FILE = "./data/ukbench.ldb"

def ukbench
  Otama.open(CONFIG_FILE) do |otama|
    Otama::KVS.open(DBM_FILE) do |db|
      begin
        otama.invoke("update_idf", 0)
      rescue
      end
      
      sum = 0
      count = 0
      qps = 0.0
      t = Time.now
      
      rdb = {}
      db.each_key do |key|
        rdb[db[key]] = key
      end
      db.each_key do |id|
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
        printf("%5d, score: %.2f, %.2f QPS\r", count, sum.to_f / count.to_f, qps)
      end
      printf("%5d/%5d, score: %.2f, %.2f QPS\n", sum, count, sum.to_f / count.to_f, qps)  
    end
  end
end
STDOUT.sync = true 
ukbench
