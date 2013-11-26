require './otama'
require 'test/unit'
require 'pp'
require 'fileutils'

FileUtils.mkdir_p("./data")

unless defined?(Otama::KVS)
  warn "Otama does not have KVS API"
  exit(0)
end

class OtamaKVSTest < Test::Unit::TestCase
  def setup
    @path = "./data/test.kvs"
    #Otama.log_level = Otama::LOG_LEVEL_DEBUG
    #Otama.log_level = Otama::LOG_LEVEL_QUIET
  end

  def test_open
    Otama::KVS.open(@path) do |kvs|
    end
    kvs = Otama::KVS.new(@path)
    kvs.close
  end
  def test_clear
    Otama::KVS.open(@path) do |kvs|
      kvs["hoge"] = "piyo"
      kvs.set("10", "yen")
      assert_equal kvs["hoge"], "piyo"
      assert_equal kvs["10"], "yen"
      assert_equal kvs.get("hoge"), "piyo"
      assert_equal kvs.get("10"), "yen"
      kvs.clear
      assert kvs["hoge"].nil?
      assert kvs["10"].nil?
      assert kvs.get("hoge").nil?
      assert kvs.get("10").nil?
    end
  end
  def test_setget
    Otama::KVS.open(@path) do |kvs|
      kvs.clear
      kvs["hoge"] = "piyo"
      kvs.set("10", "yen")
      assert_equal kvs["hoge"], "piyo"
      assert_equal kvs["10"], "yen"
      assert_equal kvs.get("hoge"), "piyo"
      assert_equal kvs.get("10"), "yen"
    end
  end
  def test_delete
    Otama::KVS.open(@path) do |kvs|
      kvs.clear
      kvs["hoge"] = "piyo"
      kvs.set("10", "yen")
      assert_equal kvs["hoge"], "piyo"
      assert_equal kvs["10"], "yen"
      assert_equal kvs.get("hoge"), "piyo"
      assert_equal kvs.get("10"), "yen"
      kvs.delete("hoge")
      assert kvs["hoge"].nil?
      assert kvs.get("hoge").nil?
      assert_equal kvs["10"], "yen"
      assert_equal kvs.get("10"), "yen"
      kvs.delete("10")
      assert kvs["10"].nil?
      assert kvs.get("10").nil?
      assert kvs["100"].nil?
      assert kvs.get("1000").nil?
    end
  end
  def test_each
    kvs = Otama::KVS.new(@path)
    begin
      kvs.clear
      kvs["1"] = "2"
      kvs["2"] = "4"
      kvs["3"] = "6"
      ksum = 0
      vsum = 0
      kvs.each do |k, v|
        assert_equal k.to_i * 2, v.to_i
        ksum += k.to_i
        vsum += v.to_i
      end
      assert_equal ksum, 6
      assert_equal vsum, 12

      ksum = 0
      vsum = 0
      
      kvs.each_key do |k|
        ksum += k.to_i
      end
      kvs.each_value do |v|
        vsum += v.to_i
      end
      assert_equal ksum, 6
      assert_equal vsum, 12
      
      c = 0
      kvs.each do |k, v|
        c += 1
        if (c > 1)
          break
        end
      end
      assert_equal c, 2
    ensure
      kvs.close
    end
  end
  def test_error
    ok = false
    begin
      kvs = Otama::KVS.new("the_not_found/the_not_found")
      kvs.close
    rescue
      ok = true
    end
    assert ok
    
    ok = false
    begin
      Otama::KVS.open("the_not_found/the_not_found") do |kvs|
        kvs.close
      end
    rescue
      ok = true
    end
    assert ok

    ok = false
    kvs = Otama::KVS.new(@path)
    begin
      kvs[1] = "10"
    rescue
      ok = true
    end
    assert ok
    
    ok = false
    begin
      kvs["10"] = 10
    rescue
      ok = true
    end
    assert ok
    
    ok = false
    begin
      kvs[10] = 1.5
    rescue
      ok = true
    end
    assert ok
    
    kvs.close
  end
end
