from locust import HttpUser, task

class IndexUser(HttpUser):
    @task
    def index(self):
        self.client.get("/index.html")
