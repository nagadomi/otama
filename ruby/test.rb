require './otama'
require 'yaml'
require 'test/unit'
require 'pp'
require 'fileutils'

if (ARGV.length == 0)
  warn "usage: #{$0} driver_config.yaml"
  exit(-1)
else
  $driver_config = ARGV[0]

  FileUtils.mkdir_p("./data")

  puts "config #{$driver_config}"
end

class OtamaTest < Test::Unit::TestCase
  FILE1  = "../image/lena.jpg"
  FILE2  = "../image/lena-nega.jpg"
  
  puts "#{__FILE__} #{Otama.version_string}"
  
  def setup
    #Otama.log_level = Otama::LOG_LEVEL_DEBUG
    #Otama.log_level = Otama::LOG_LEVEL_QUIET
    
    @drop_create = lambda do
      Otama.open($driver_config) do |otama|
        otama.drop_database
        otama.create_database
      end
    end
    begin
      Otama.open($driver_config) do |otama|
        otama.create_database
      end
    rescue
    end
  end
  def test_open
    Otama.open($driver_config) do |otama|
    end
    otama = Otama.open($driver_config)
    assert_equal otama.active?, true
    otama.close
    
    exception = false
    begin
      otama = Otama.open("driver" => {"name" => "DUMMYDUMMYDUMMY"})
    rescue
      exception = true
    end
    assert_equal exception, true
  end
  
  def test_create_drop_create
    Otama.open($driver_config) do |otama|
      i = 0
      begin
        otama.create_database
        i = 1
        otama.drop_database
        otama.create_database
        i = 2
      rescue
        if (i == 0)
          otama.drop_database
          otama.create_database
          otama.drop_database
          otama.create_database
          i = 2
        end
      end
      assert_equal i, 2
      assert_equal otama.count, 0
    end
  end
  def test_id
    @drop_create.call    
    Otama.open($driver_config) do |otama|
      id1 = otama.insert(:file => FILE1)
      id2 = otama.insert(:file => FILE2)
      assert_equal id1, Otama.id(:file => FILE1)
      assert_equal id2, Otama.id(:data => File.open(FILE2, "rb").read)
      assert_equal id1, Otama.id("file" => FILE1)
      assert_equal id2, Otama.id("data" => File.open(FILE2, "rb").read)
    end
  end
  def test_insert_pull
    @drop_create.call
    Otama.open($driver_config) do |otama|
      id1 = otama.insert("file" => FILE1)
      id2 = otama.insert("file" => FILE2)
      otama.pull
      assert_equal otama.count, 2
    end
  end
  def test_file_insert_search_remove_search
    @drop_create.call
    Otama.open($driver_config) do |otama|
      id1 = otama.insert(:file => FILE1)
      id2 = otama.insert(:file => FILE2)
      otama.pull
      
      results = otama.search(10, {:file => FILE1})
      assert results.size > 0
      assert_equal results[0].id, id1
      results = otama.search(10, {:file => FILE2})
      assert results.size > 0
      assert_equal results[0].id, id2

      otama.remove(id1)
      otama.pull
      
      results = otama.search(10, {:file => FILE2})
      assert results.size > 0
      assert_equal results[0].id, id2

      otama.remove(id2)
      otama.pull
      
      results = otama.search(10, :file => FILE1)
      assert_equal results.size, 0
      results = otama.search(10, :file => FILE2)
      assert_equal results.size, 0
    end
  end
  def test_error
    @drop_create.call    
    error = false
    begin
      otama.insert(:hogehoge => File.open(FILE1, "rb").read)
    rescue
      error = true
    end
    assert_equal error, true
    
    error = false
    begin
      otama.insert(nil)
    rescue
      error = true
    end
    assert_equal error, true

    begin
      otama.search(:hogehoge => File.open(FILE1, "rb").read)
    rescue
      error = true
    end
    assert_equal error, true
    
    error = false
    begin
      otama.search(nil)
    rescue
      error = true
    end
    assert_equal error, true


    error = false
    begin
      otama.search(:id => "hogehoge")
    rescue
      error = true
    end
    assert_equal error, true
  end
  def test_data_insert_search_remove_search
    @drop_create.call
    Otama.open($driver_config) do |otama|
      id1 = otama.insert(:data => File.open(FILE1, "rb").read)
      id2 = otama.insert(:data => File.open(FILE2, "rb").read)
      otama.pull
      
      results = otama.search(10, :data => File.open(FILE1, "rb").read)
      assert results.size > 0
      assert_equal results[0].id, id1
      results = otama.search(10, :data => File.open(FILE2, "rb").read)
      assert_equal results[0].id, id2
      
      otama.remove(id1)
      otama.pull
      
      results = otama.search(10, :data => File.open(FILE2, "rb").read)
      assert results.size > 0
      assert_equal results[0].id, id2
      
      otama.remove(id2)
      otama.pull
      
      results = otama.search(10, :data => File.open(FILE1, "rb").read)
      assert_equal results.size, 0
      
      results = otama.search(10, :data => File.open(FILE2, "rb").read)
      assert_equal results.size, 0      
    end
  end
  def test_string_insert_search_remove_search
    @drop_create.call
    otama = Otama.open($driver_config)
    begin
      id1 = otama.insert(:file => FILE1)
      id2 = otama.insert(:file => FILE2)
      otama.pull
      
      fs1 = otama.feature_string(:file => FILE1)
      fs2 = otama.feature_string(:data => File.open(FILE2, "rb").read)

      results = otama.search(10, :string => fs1)
      assert results.size > 0      
      assert_equal results[0].id, id1
      results = otama.search(10, :string => fs2)
      assert results.size > 0
      assert_equal results[0].id, id2
      
      otama.remove(id1)
      
      otama.pull
      results = otama.search(10, :string => fs2)
      assert results.size > 0      
      assert_equal results[0].id, id2

      otama.remove(id2)
      otama.pull
      
      results = otama.search(10, :string => fs1)
      assert_equal results.size, 0
      results = otama.search(10, :string => fs2)
      assert_equal results.size, 0
    ensure
      otama.close
    end
  end
  def test_raw_insert_search_remove_search
    @drop_create.call
    otama = Otama.open($driver_config)
    begin
      id1 = otama.insert(:file => FILE1)
      id2 = otama.insert(:file => FILE2)
      otama.pull
      
      fs1 = otama.feature_raw(:file => FILE1)
      fs2 = otama.feature_raw(:data => File.open(FILE2, "rb").read)

      results = otama.search(10, :raw => fs1)
      assert results.size > 0      
      assert_equal results[0].id, id1
      results = otama.search(10, :raw => fs2)
      assert results.size > 0
      assert_equal results[0].id, id2
      
      otama.remove(id1)
      
      otama.pull
      results = otama.search(10, :raw => fs2)
      assert results.size > 0      
      assert_equal results[0].id, id2

      otama.remove(id2)
      otama.pull
      
      results = otama.search(10, :raw => fs1)
      assert_equal results.size, 0
      results = otama.search(10, :raw => fs2)
      assert_equal results.size, 0
    ensure
      otama.close
    end
  end
  def test_id_insert_search_remove_search
    @drop_create.call
    otama = Otama.open($driver_config)
    begin
      id1 = otama.insert(:file => FILE1)
      id2 = otama.insert(:file => FILE2)
      otama.pull
      
      results = otama.search(10, :id => id1)
      assert results.size > 0
      assert_equal results[0].id, id1
      results = otama.search(10, :id => id2)
      assert results.size > 0
      assert_equal results[0].id, id2
      
      otama.remove(id1)
      
      otama.pull
      results = otama.search(10, :id => id2)
      assert results.size > 0
      assert_equal results[0].id, id2

      otama.remove(id2)
      otama.pull
      
      results = otama.search(10, :id => id1)
      assert_equal results.size, 0
      results = otama.search(10, :id => id2)
      assert_equal results.size, 0
    ensure
      otama.close
    end
  end
end
