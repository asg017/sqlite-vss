from datasette.app import Datasette
import pytest


@pytest.mark.asyncio
async def test_plugin_is_installed():
    datasette = Datasette(memory=True)
    response = await datasette.client.get("/-/plugins.json")
    assert response.status_code == 200
    installed_plugins = {p["name"] for p in response.json()}
    assert "datasette-sqlite-vector" in installed_plugins

@pytest.mark.asyncio
async def test_sqlite_vector_functions():
    datasette = Datasette(memory=True)
    response = await datasette.client.get("/_memory.json?sql=select+vector_version(),vector()")
    assert response.status_code == 200
    vector_version, vector = response.json()["rows"][0]
    assert vector_version[0] == "v"
    assert len(vector) == 26