require './otama'
require 'yaml'
require 'test/unit'
require 'pp'
require 'fileutils'

if (ARGV.length == 0)
  warn "usage: #{$0} driver_config.yaml [shm_dir]"
  exit(-1)
else
  $driver_config = ARGV[0]

  FileUtils.mkdir_p("./data")

  puts "config #{$driver_config}"
end

class OtamaScoreTest < Test::Unit::TestCase
  FILE1  = File.join(File.dirname(__FILE__), "../image/lena.jpg")
  FILE2  = File.join(File.dirname(__FILE__), "../image/lena-nega.jpg")
  
  def setup
    #Otama.log_level = Otama::LOG_LEVEL_DEBUG
    #Otama.log_level = Otama::LOG_LEVEL_QUIET
  end

  def test_open
    Otama.open($driver_config) do |otama|
    end
    otama = Otama.open($driver_config)
    begin
      assert_equal otama.active?, true
    ensure
      otama.close
    end
  end

  def test_similarity
    Otama.open($driver_config) do |otama|
      similarity1 = otama.similarity({:file => FILE1}, {:file => FILE2})
      assert similarity1.is_a?(Float)
      similarity2 = otama.similarity({:data => File.open(FILE1, "rb").read},
                          {:data => File.open(FILE1, "rb").read})
      assert similarity2.is_a?(Float)
      assert similarity1 < similarity2
    end
  end
  def test_loop
    Otama.open($driver_config) do |otama|
      (0 .. 4).each do |i|
        similarity1 = otama.similarity({:file => FILE1}, {:file => FILE2})
        assert similarity1.is_a?(Float)
        similarity2 = otama.similarity({:data => File.open(FILE1, "rb").read},
                             {:data => File.open(FILE1, "rb").read})
        assert similarity2.is_a?(Float)
        assert similarity1 < similarity2
      end
    end
  end
  def test_feature_string
    otama = Otama.open($driver_config)
    begin
      fs1 = otama.feature_string(:file => FILE1)
      fs2 = otama.feature_string(:data => File.open(FILE2, "rb").read)

      similarity1 = otama.similarity({:string => fs1}, {:string => fs2})
      assert similarity1.is_a?(Float)
      similarity2 = otama.similarity({:data => File.open(FILE1, "rb").read},
                           {:data => File.open(FILE1, "rb").read})
      assert similarity2.is_a?(Float)
      assert similarity1 < similarity2
    ensure
      otama.close
    end
  end
  def test_feature_raw
    otama = Otama.open($driver_config)
    begin
      fs1 = otama.feature_raw(:file => FILE1)
      fs2 = otama.feature_raw(:data => File.open(FILE2, "rb").read)

      similarity1 = otama.similarity({:raw => fs1}, {:raw => fs2})
      assert similarity1.is_a?(Float)
      similarity2 = otama.similarity({:data => File.open(FILE1, "rb").read},
                           {:data => File.open(FILE1, "rb").read})
      assert similarity2.is_a?(Float)
      assert similarity1 < similarity2
    ensure
      otama.close
    end
  end
end
