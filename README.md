# Render farm

```sh
# install openfaas inside kubernetes cluster
arkade install openfaas

# check that gateway is ready
kubectl rollout status -n openfaas deploy/gateway 

# forward gateway to be accessible outside cluster (create external gateway with Nodeport) : admin / password (see below)
kubectl port-forward svc/gateway -n openfaas 8080:8080

# get password for ui access
kubectl get secret -n openfaas basic-auth -o jsonpath="{.data.basic-auth-password}" | base64 --decode; echo

# run grafana inside the cluster
kubectl -n openfaas run --image=stefanprodan/faas-grafana:4.6.3 --port=3000 grafana
kubectl -n openfaas expose pod grafana --type=NodePort --name=grafana

# forward grafana to be accessible outside cluster : admin / admin
kubectl port-forward pod/grafana 3000:3000 -n openfaas

# forward prometheus to be accessible outside cluster : admin / admin
kubectl port-forward deployment/prometheus 9090:9090 -n openfaas
# use this query to display all successful invocations
rate( gateway_function_invocation_total{code="200"} [20s])

# run load testing
hey -z=30s -q 5 -c 2 -m POST -d=Test http://127.0.0.1:8080/function/go-echo

# apply gateway timeouts patch to gateway deployment
kubectl patch deployment gateway -n openfaas --patch-file gateway-timeouts-patch.yaml

# apply faas-netes timeouts patch to gateway deployment
kubectl patch deployment gateway -n openfaas --patch-file faas-netes-timeouts-patch.yaml

# restart deployment to apply changes
kubectl rollout restart deployment gateway -n openfaas

# check that timeouts have been properly updated
kubectl get deployment gateway -n openfaas -o yaml | grep -A10 'env:' 

# build image
docker build -t zephyr75/render_farm:latest .

# push image to dockerhub
docker push zephyr75/render_farm:latest

# optional : run docker outside openfaas
docker run -p 8082:8080 -t zephyr75/render_farm:latest

# deploy dockerhub image to openfaas
faas-cli deploy -f stack.yaml

faas-cli build -f stack.yaml
faas-cli push -f stack.yaml
faas-cli deploy -f stack.yaml


# reconnect to openfaas on managed kube with load balancer
kubectl rollout status -n openfaas deploy/gateway

# reconnect to openfaas on local kube
kubectl port-forward svc/gateway -n openfaas 8080:8080

# get ip of openfaas on managed kube with load balancer
kubectl get svc -o wide gateway-external -n openfaas

```
