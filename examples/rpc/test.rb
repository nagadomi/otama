require 'rubygems'
require './otama_client'
require 'test/unit'

class OtamaTest < Test::Unit::TestCase
  OTAMA_API = 'http://localhost:4568/'
  FILE1  = "../../image/lena.jpg"
  FILE2  = "../../image/lena-nega.jpg"
  
  def setup
    @client = OtamaClient.new(OTAMA_API)
  end
  def test_all
    res = @client.insert(:file => FILE1)
    assert res
    assert res["status"].to_i == 200
    assert res["data"]
    assert res["data"]["id"]
    id1 = res["data"]["id"]
    res = @client.insert(:data => File.open(FILE2).read)
    
    assert res
    assert res["status"].to_i == 200
    assert res["data"]
    assert res["data"]["id"]
    id2 = res["data"]["id"]
    
    res = @client.pull
    assert res
    assert res["status"].to_i == 200
    
    res = @client.search(:id => id1)
    assert res
    assert res["data"]
    assert res["data"].size > 0
    assert res["data"][0]["id"] == id1

    res = @client.search(:file => FILE2)
    assert res
    assert res["status"].to_i == 200
    assert res["data"]
    assert res["data"].size > 0
    assert res["data"][0]["id"] == id2
    
    res = @client.search(:data => File.open(FILE1).read)
    assert res
    assert res["status"].to_i == 200    
    assert res["data"]
    assert res["data"].size > 0
    assert res["data"][0]["id"] == id1

    res = @client.remove(id1)
    res = @client.pull
    res = @client.search(:id => id1)
    assert res
    assert res["status"].to_i == 200
    assert res["data"]
    if (res["data"].size > 0)
      assert res["data"][0]["id"] != id1
    end
    
    res = @client.remove(id2)
    res = @client.pull
    res = @client.search(:id => id2)
    assert res
    assert res["status"].to_i == 200
    assert res["data"]
    if (res["data"].size > 0)
      assert res["data"][0]["id"] != id2
    end
  end
end
